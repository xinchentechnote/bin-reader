# bin-reader 设计文档

> 面向维护者的架构说明：模块边界、三条主链路、状态不变量、扩展方式与验证手段。
> 待办与优化方向见 [backlog.md](backlog.md)，命令语义见 [cmd.md](cmd.md)。

## 1. 定位与约束

交互式终端二进制查看器：把整个文件读进内存，用光标（字节偏移）定位，通过命令
按类型读取数据并累积成可撤销的读取历史。

设计上的三条硬约束：

- **单文件、全量载入**：`Utils::read_binary_file()` 一次性读完，所有边界判断都以
  `data.size()` 为准。不做分页/mmap，因此内存占用 ≈ 文件大小（见 backlog 性能项）。
- **只读语义**：当前不存在写路径，`w`/`save` 只是 cmd.md 里的设计草案。
- **一帧一状态**：渲染层每帧从 `AppState` 重新拉取全部数据，不做增量缓存。

## 2. 模块地图

依赖方向自上而下，`AppState` 是唯一的可变状态载体：

```
main.cpp
  └─ UIComponents   (组件树 + 每帧渲染，只读 AppState)
       ├─ EventHandlers  (键盘事件 → 命令/导航)
       │     └─ Command  (文本命令解析 + 注册表)
       │            └─ AppState  (状态 + 读取原语 + ReaderFactory)
       └─ Utils      (无状态工具：文件读取、数值格式化、类型串解析)
```

| 文件 | 职责 | 关键符号 |
|---|---|---|
| `src/main.cpp` | 装配：解析 CLI → 注册命令 → 建屏 → 载入文件 → `screen.Loop(ui)` | `main` |
| `src/AppState.hpp` | 状态结构体、读取原语、类型名表、读取策略与工厂声明 | `AppState` L131、`Record` L111、`TypeTraits`/`TypeFactory` L70、`ReaderStrategy` 家族 L424 之前 |
| `src/AppState.cpp` | `ReaderFactory` 单例：类型串 → 读取策略 | `ReaderFactory::read` L41 |
| `src/Command.hpp` | 导航命令枚举 `CommandType` + 文本命令 `ParsedCommand` + `CommandRegistry` | `ParsedCommand::parse` |
| `src/Command.cpp` | 具体命令实现：`r j u be le q quit` | `register_all_commands` L5 |
| `src/EventHandlers.cpp` | 键盘事件分流：回车走命令表，方向键走导航 | `HandleNavigation` L13、`HandleCommands` L48 |
| `src/UIComponents.cpp` | 四个面板组件 + 组合根 + 初始焦点 | `MainUi`、`HexView`、`DataPreviewBar`、`DataReadHistoryBar`、`CommandLine` |
| `src/Utils.hpp` | 文件读取、`format_value`/`format_any`、`char[n]` 长度解析 | `Utils::*` |
| `include/CLI11.hpp` |  vendored 单头命令行解析库（v2.4.2） | `Utils::ParseCommandLine` |

`AppState.hpp` 目前承载 5 个概念（类型表 / Record / 状态 / 策略 / 工厂），是最重的
一个头文件；拆分计划见 backlog「工程性」。

## 3. 运行时序

```
main
 ├─ Utils::ParseCommandLine(argc, argv)      解析 -f/--file，CLI11 校验文件存在
 ├─ register_all_commands()                  往 CommandRegistry 单例灌入命令闭包
 ├─ AppState state                           全程序唯一状态实例（栈上，非堆）
 ├─ state.load_file(path)                    读文件 + 复位 cursor/page
 ├─ UIComponents::MainUi(state, cmd, screen) 组装组件树，并同步 TakeFocus
 └─ screen.Loop(ui)                          进入 FTXUI 事件循环，直到 Exit()
```

`cmd`（输入框绑定的字符串）与 `state` 一起被闭包捕获，是 UI 与命令层之间唯一的
共享可变量。

## 4. 三条主链路

### 4.1 事件链（键盘 → 动作）

FTXUI 把每个 `Event` 交给根组件 `CatchEvent`，回调返回值语义是**拦截**：

- 返回 `true`：事件被消费，**不再**向下传递给组件树（`Input` 就收不到）。
- 返回 `false`：继续向下传递，由获得焦点的子组件处理。

`MainUi` 的分流顺序（`src/UIComponents.cpp` L50-63）：

```
Event
 └─ CatchEvent 回调
      ├─ EventHandlers::HandleCommands   仅处理 Event::Return；其余返回 false
      └─ EventHandlers::HandleNavigation '.' 重复上次导航 / Escape 退出 /
                                          六个方向与翻页键 → 返回 true
                                          其它事件 → 返回 false（交给 Input）
```

因此普通字符键（含退格）会穿过两层回调，最终落到拥有焦点的 `Input`；
方向键被导航层占用，`Input` 拿不到编辑光标移动。

> 陷阱记录：导航层曾经对任意事件都返回 `true`，导致所有键入被拦截，命令行彻底
> 失效。判断依据是「回车后 `ParsedCommand::name` 为空」。

### 4.2 焦点模型

`Container::Vertical({status_bar, workspace, command_line})` 默认把焦点给第 0 个
子节点。必须让命令行获得焦点，`Input` 才会收到键入：

```cpp
// src/UIComponents.cpp L57-61
auto ui = CatchEvent(layout, ...);
command_line->TakeFocus();   // 同步调用，不能靠 PostEvent
return ui;
```

`TakeFocus()` 沿父链把每一层的 active child 指到自己（FTXUI
`ComponentBase::TakeFocus` 的实现），因此必须在整棵树（含 `CatchEvent`）组装完成
之后调用。

> 陷阱记录：原实现是 `screen.PostEvent(Event::Custom)` + 在回调里 `TakeFocus()`。
> `ScreenInteractive::Post` 在屏幕尚未激活时**直接丢弃**任务（源码注释：
> "Task/Events sent toward inactive screen or screen waiting to become inactive are
> dropped"），而 `PostEvent` 发生在 `screen.Loop()` 之前，于是焦点永远没有转移过。

### 4.3 命令链（文本 → 状态变更）

```
command_input ("r u16")
  → ParsedCommand::parse        按空白切分：name="r", args=["u16"]
  → CommandRegistry::dispatch   查表调用闭包(AppState&, const ParsedCommand&)
  → 闭包修改 state 并写 state.status_msg
  → command_input.clear()
```

命令表（`src/Command.cpp`，实现状态详见 cmd.md）：

| 命令 | 行为 | 失败时的 status_msg |
|---|---|---|
| `r <type>` | 按类型读取并推进光标 | `Usage: ...` / `Unknown type: X` / `X read failed: <原因>` |
| `j <addr>` | 绝对跳转（十进制或 `0x` 十六进制） | `Invalid jump value.` / `Invalid absolute position!` / `Invalid hex position!` |
| `j ±N` | 相对跳转，允许落在偏移 0 | `Invalid relative position!` |
| `j` | 显示当前位置 | `Current position: 0x…` |
| `u` | 撤销最近一次读取 | `Nothing to undo.` |
| `be` / `le` | 切换字节序（影响后续所有多字节读取与预览） | `Big-endian mode` / `Little-endian mode` |
| `q` / `quit` | 置 `exit_requested`，由事件层调用 `screen.Exit()` | — |

`CommandRegistry::dispatch` 未命中时只返回 `false`，"Unknown command" 的提示文本
由 `HandleCommands` 生成——提示语与分发解耦。

### 4.4 读取链（类型串 → 字节 → 历史）

```
ReaderFactory::read(state, "u16" | "char[10]" | "string@u8" | ...)   AppState.cpp L41
  ├─ 精确查表：u8..f64、char[n] 走 "fixstring"、string@{u8,u16,u32,i8,i16,i32}
  ├─ 未命中 → status_msg = "Unknown type: X"，返回 false
  └─ try { strategy->read(state, type) } catch (std::exception)
        → status_msg = "<type> read failed: <what>"，返回 false
```

策略实现分工：

- `TypedReader<T>` → `AppState::read<T>()` → `peek<T>(cursor_pos)` + 记录 + `move`
- `FixStringReader` → `Utils::parse_char_length(type)` → `read_fixed_string(n)`
- `LengthPrefixedStringReader<L>` → `read_length_prefixed_string<L>()`
  （先 `peek<L>` 取长度，再取正文，**只产生一条**合并记录）

异常只在 `ReaderFactory` 这一层统一收敛，策略本身不需要 `try/catch`——新增策略
只要保证「抛异常 = 零副作用」即可。

## 5. 状态模型与不变量

`AppState`（`src/AppState.hpp` L131）字段：

| 字段 | 语义 |
|---|---|
| `data` | 文件全量字节，载入后不再变化 |
| `cursor_pos` | 当前字节偏移，**下一次读取的起点** |
| `bytes_per_line` | hex 视图每行字节数，默认 16 |
| `hex_view_h` | 每页行数，默认 16（当前为固定值，未随终端尺寸变化） |
| `current_page` | 由 `cursor_pos` 反推：`(pos / bytes_per_line) / hex_view_h` |
| `status_msg` | 状态栏唯一文案载体，命令层负责写入 |
| `is_little_endian` | 默认 `true`；`be/le` 切换，作用于 `peek` 的字节反转 |
| `read_history` | `std::vector<Record>`，按读取先后顺序追加 |
| `last_command` | 最近一次键盘导航命令，供 `.` 重复 |

维护的不变量：

1. `cursor_pos <= data.size()`；`<=` 是刻意的——允许「文件末尾」这个哨兵位置
   （读完最后一个记录后光标自然越出最后一个字节），但 `peek`/`j` 的绝对地址必须
   严格 `< data.size()`。
2. **读取失败不留副作用**：任何 `out_of_range` 抛出前不得修改 `cursor_pos`，也不得
   向 `read_history` 追加记录。长度前缀字符串尤其要注意（曾用「读两条再弹两条」
   实现，失败时残留脏记录）。
3. `read_history` 与 `cursor_pos` 自洽：`undo()` 弹出末条并把光标恢复到该条的
   `index`；因此 `undo` 之后的历史一定是「当前光标之前所有已确认读取」。
4. `current_page` 与 `cursor_pos` 同步：所有改动偏移的路径都必须经过
   `set_cursor_pos()` 或 `move()`（两者最终都调用 `update_page()`）。

`Record`：`index`（起始偏移）+ `data`(`std::any`) + `type_name`（展示用短名，
如 `u16`、`char[10]`、`string@u8`）。`Utils::format_any` 用 `type()` 分派展示格式。

## 6. 类型系统

两套并行的名字表，别混淆：

- **`TypeTraits<T>`**：编译期把 C++ 类型映射到 `name`（`uint16_t`）与
  `short_name`（`u16`）。命令与展示都用 `short_name`。
- **`TypeFactory`**：单例，构造时把两种名字都注册进 `type_map_`（`type_index`）。
  目前外部只用到静态模板 `getTypeShortName<T>()`，`type_map_` 实际未被读取
  （backlog 死代码项）。
- **`ReaderFactory`**：运行期把「类型串」映射到读取策略。字符串类型是**语法糖**而
  非模板参数：`char[n]` 走 `is_char_array_type` + `parse_char_length`，
  `string@<int>` 走显式注册的六种前缀类型。

支持的类型串：

```
u8 i8 u16 i16 u32 i32 u64 i64 f32 f64
char[<n>]                     定长字符串，n 为十进制
string@{u8,u16,u32,i8,i16,i32} 长度前缀字符串（无 u64/i64）
```

大小端只作用于定长数值（`peek<T>` 里按 `sizeof(T)` 反转），对字符串无语义。
注意实现假定**宿主是小端**（详见 backlog 可移植性）。

## 7. 渲染模型

四个区域，全部由 `Renderer` 闭包每帧重算：

```
┌ StatusBar ────────────────────────────────────────────────┐  Pos / Page / status_msg / File
┌ HexView (flex) ──────┐ DataPreviewBar ┐ DataReadHistoryBar ┐  地址+hex+ASCII / 光标处各类型解释 / 读取历史
└ CommandLine ──────────────────────────────────────────────┘  ">|" + Input
```

- `HexView` 渲染 `[current_page*hex_view_h, ...)` 共 `hex_view_h` 行，每行 16 字节；
  当前光标单元格 `inverted`，不可打印 ASCII 显示为 `.`。
- `DataPreviewBar` 对光标处 10 种数值类型做 `peek`，越界显示 `N/A`，并显示 `LE/BE`。
- `DataReadHistoryBar` 直接引用 `state.get_read_history()`（返回 `const&`，不做拷贝）。
- 面板宽度是硬编码约束（hex 150、两侧各 35，内部又各自设 30），终端不足时会被压缩
  并裁剪；高度按 `hex_view_h` 固定，未监听 `Event::Resize`。这两条是 backlog 的
  「响应式布局」项，实测 120 列终端下第三个面板已被截断。

性能特征：每帧 O(已读记录数)。历史上 `get_read_history()` 曾用 `insert(begin())`
重建 vector，4000 条记录时单次调用 516 ms（每帧一次），现已改为返回引用。虚拟滚动
仍未实现。

## 8. 扩展指南

**加一个数值类型**：`TypeTraits<T>` 特化 → `TypeFactory::registerType<T>()` →
`ReaderFactory` 构造函数里 `emplaceReader<T>()`。若 `T` 不是平凡可拷贝类型，
需要新写一个策略类而不是复用 `TypedReader<T>`（它依赖 `memcpy` 语义）。

**加一个字符串族类型**（例：`string@u64`）：在 `ReaderFactory()` 里
`readers_.emplace("string@u64", make_unique<LengthPrefixedStringReader<uint64_t>>())`。

**加一条命令**：在 `register_all_commands()` 里注册闭包即可；约定是
「命令只改状态并写 `status_msg`，不做退出」——退出通过 `state.exit_requested`
交给 `HandleCommands`（保持命令层不依赖 FTXUI）。

**新增策略类**：实现 `ReaderStrategy::read(AppState&, const std::string&)`，
遵守「失败即抛、抛前不改状态」，剩下的错误提示由工厂统一负责。

## 9. 测试与验证

- 单元测试 34 例（`tests/`）：状态原语与边界、ReaderFactory 各类型、命令注册表
  行为、事件层按键归属、hex 视图渲染快照。
  `cmake -B build -DBUILD_TESTING=ON && cmake --build build && ctest --test-dir build`
- **端到端必须人工跑一次 TUI**：单测无法覆盖「事件能否穿过 `CatchEvent` 到达
  `Input`」，而这正是历史上最致命的缺陷所在。做法是起一个 pty，把按键写进去，
  再解析输出里的状态栏文本。注意 FTXUI 启动时会发送 DCS 查询
  （`\x1bP$q q\x1b\\`），简易终端模拟器必须跳过它，否则其载荷会被当成正文渲染。
- 回归关注点：键入可见 → 回车生效 → `.` 重复导航 → 退格可用 → `be/le` 读值变化
  （`tests/data/HelloWorld.class` 前 4 字节 `CA FE BA BE`，`be` 下 `r u32` 应为
  3405691582，`le` 下应为 3199925962）。

## 10. 构建与依赖

- CMake ≥ 3.15，C++17，`-Wall -Wextra -Wpedantic -Werror`。
- 依赖用 `FetchContent` 拉取并固定版本：FTXUI `v6.1.9`、fmt `8.1.1`、
  googletest `v1.15.0`；下载缓存目录被设为 `$HOME/_deps`
  （`FETCHCONTENT_BASE_DIR`，为的是跨工程复用，但它是**工程目录之外的写入**）。
  CLI11 是例外——手工 vendored 在 `include/`。
- 输出目录：`build/bin/bin-reader`、`build/bin/bin-reader-tests`。
- 顶层 `if(BUILD_TESTING)` 之前没有 `include(CTest)`，所以该开关未被声明为默认
  `ON`：不显式传 `-DBUILD_TESTING=ON` 时测试目标会被静默跳过。
- 对 FTXUI 的 `dom`/`screen` 目标单独关掉 `-Werror=pedantic`，以避开第三方代码
  在 `-Wpedantic` 下的告警。
