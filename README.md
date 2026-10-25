# Bin-Reader

![Demo](demo.gif) *(todo)*

一个基于 **C++17** 和 **FTXUI** 的交互式二进制文件解析工具：在终端里按类型逐条读取字节流，随时撤销，配合 hex/ascii 双视图定位偏移。

设计文档见 [docs/design.md](docs/design.md)，命令语法设计见 [docs/cmd.md](docs/cmd.md)，缺陷与优化清单见 [docs/backlog.md](docs/backlog.md)。

## 功能特性

- **交互式 TUI**：顶部状态栏（偏移 / 页码 / 上一条输出 / 文件名），中部三栏（hex+ascii 区 / 当前字节的多类型预览 / 已读历史），底部命令行。
- **基本类型读取**：`u8 i8 u16 i16 u32 i32 u64 i64 f32 f64`。
- **字符串类型**：`char[n]` 定长字符串、`string@u8|u16|u32|i8|i16|i32` 长度前缀字符串（前缀与内容合并为一条历史记录）。
- **字节序切换**：`be` / `le`，读取时按目标字节序重排。
- **历史与回滚**：所有读取进入历史记录，`u` 撤销上一步并把光标移回该记录起始偏移。
- **失败无副作用**：越界、类型未知、长度前缀非法都不会污染光标或历史，只在状态栏给出可行动的错误信息。
- **导航**：方向键按字节/按行移动，`PageUp`/`PageDown` 翻页，`.` 重复上一条导航动作。
- **退出**：`q` / `quit` / `Esc`。

## 构建与运行

依赖（FTXUI v6.1.9、fmt 8.1.1、GoogleTest v1.15.0）由 CMake `FetchContent` 自动拉取，无需手动安装；CLI11 以单头文件形式随源码放在 `include/`。

```bash
git clone https://github.com/xinchentechnote/bin-reader.git
cd bin-reader

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/bin/bin-reader -f path/to/your/file.bin
```

可执行文件输出在 `build/bin/bin-reader`（由 `CMAKE_RUNTIME_OUTPUT_DIRECTORY` 决定），不是项目根目录。

注意两点：

- `FetchContent` 的缓存目录被设为 `$HOME/_deps`，构建会写入项目之外的这个目录。
- 测试目标位于 `if(BUILD_TESTING)` 分支内，而 `include(CTest)` 也在同一分支里，所以全新配置时必须显式打开：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build
cd build && ctest --output-on-failure
```

`build.sh` 封装了以上完整流程（含 `ctest` 与一次样例运行），但它会修改全局 `git config`，首次使用请先读一遍脚本。

## 命令

命令在底部输入框输入，回车执行。

| 命令 | 说明 | 状态栏输出示例 |
| --- | --- | --- |
| `r <type>` | 从当前光标读取一个 `<type>` | `Read u16: 9 @ 0x2` |
| `j <addr>` | 跳到绝对地址，支持 `0x` 前缀与十进制 | `Position set to 0xA` |
| `j +<n>` / `j -<n>` | 相对当前光标前进/后退 `n` 字节 | `Jumped -2 bytes to 0x6` |
| `j` | 不带参数时显示当前位置 | `Current position: 0x0` |
| `u` | 撤销上一条读取，光标回到该记录起始处 | `Undo to offset: 0x4` |
| `be` / `le` | 切换大端 / 小端 | `Big-endian mode` |
| `q` / `quit` | 退出（`exit` 未注册） | `Unknown command: exit` |

键盘（不进入输入框，直接生效）：

| 按键 | 作用 |
| --- | --- |
| `←` / `→` | 光标前进/后退 1 字节 |
| `↑` / `↓` | 光标上/下一行（`bytes_per_line`，默认 16 字节） |
| `PageUp` / `PageDown` | 上/下一页（每页 `hex_view_h` 行，默认 16 行） |
| `.` | 重复上一条导航动作 |
| `Esc` | 退出 |

错误信息都在状态栏显示，实际文案如下：

- 类型未注册：`Unknown type: nosuch`
- 读取越界：`u64 read failed: Attempt to read beyond data bounds`
- 长度前缀字符串超出文件：`string@u8 read failed: Length-prefixed string exceeds data size`
- 命令未实现：`Unknown command: info`
- `r` 缺参数：`Usage: r <type>  (u8 i8 u16 i16 u32 i32 u64 i64 f32 f64 char[n] string@u8)`
- 地址非法：`Invalid jump value.`
- 地址越界：`Invalid hex position!` / `Invalid absolute position!`
- 无可撤销记录：`Nothing to undo.`

## 使用示例

下面是仓库内测试数据上的真实会话（状态栏即工具的输出）。

```
$ ./build/bin/bin-reader -f tests/data/msg.bin

> be                 # 切换大端
Big-endian mode
> r u16              # 消息类型
Read u16: 1 @ 0x0
> r u16              # 载荷长度 = 9
Read u16: 9 @ 0x2
> r char[9]          # 定长读取载荷
Read char[9]: {"a":123} @ 0x4
> u                  # 撤销，光标回到该记录起始处
Undo to offset: 0x4
> j -4               # 相对跳转
Jumped -4 bytes to 0x0
```

```
$ ./build/bin/bin-reader -f tests/data/HelloWorld.class

> be
Big-endian mode
> r u32              # 0xCAFEBABE 魔数
Read u32: 3405691582 @ 0x0
> r u16              # minor_version
Read u16: 0 @ 0x4
> r u16              # major_version = 0x3D
Read u16: 61 @ 0x6
```

## 已知限制

以下都在 [docs/backlog.md](docs/backlog.md) 里按优先级登记，这里只列最影响日常使用的几条：

- `r <type> [N] [@addr]` 的批量数量与指定地址 **尚未实现**，多余参数被静默忽略：`r u16 3` 实测只读一个 u16（光标 +2）。需要先 `j <addr>` 再 `r <type>`。
- 未实现：`@`（显示上下文）、`/`（搜索）、`w`（写入）、`save`、`help`、`config`、`exit`（用 `q`/`quit`）、批处理 `-e`；`docs/cmd.md` 里这些标注为「设计中」。至于旧 README 提到的 `info`、`list`、`offset`，它们从未实现过，功能分别由状态栏、历史面板与 `j` 承担。
- 整个文件一次性读入内存，未做 mmap 或分段加载；历史面板每帧全量重建。
- 未处理终端 `Event::Resize`，三栏宽度为固定值。

## 协议

MIT License. 详情见 [LICENSE](LICENSE).
