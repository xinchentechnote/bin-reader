# bin-reader 待办与优化方向

图例：**P0** 阻断使用 · **P1** 结果错误/明显性能问题 · **P2** 语义与体验缺陷 ·
**P3** 可维护性 · `S` ≈ 半小时级 · `M` ≈ 数小时 · `L` ≈ 需要设计

架构说明见 [design.md](design.md)，命令语义见 [cmd.md](cmd.md)。

---

## 一、本轮已修复（含验证方式）

| 级别 | 问题 | 位置 | 验证 |
|---|---|---|---|
| P0 | 命令行完全无法输入：导航处理器对任意事件返回 `true`，`CatchEvent` 因此拦截全部按键；且 `MainUi` 用 `PostEvent` 转交焦点，而屏幕未激活时 `Post` 会静默丢事件，焦点一直停在 `status_bar` | `EventHandlers.cpp:13` `UIComponents.cpp:50-63` | pty 实测：`r u16`→`Read u16: 256 @ 0x0` 且 `Pos: 0x2`；`EventHandlersTest.OnlyNavigationKeysAreIntercepted` |
| P1 | 长度前缀字符串读取失败后 `read_history` 残留脏记录（「读两条再弹两条」的实现只在成功路径自洽） | `AppState.hpp:read_length_prefixed_string` | `AppStateTest.ReadLengthPrefixedString` 断言失败后历史为空 |
| P1 | `j -N` 永远无法落到偏移 0：`move(size_t)` 里 `new_pos > 0` 判定，加上把负数强转 `size_t` 的回绕 | `AppState.hpp:move` | `CommandTest.JumpRelativeBackwardsReachesZero`；pty `j -4`→`Jumped -4 bytes to 0x9` |
| P1 | 每个渲染帧全量拷贝历史并用 `insert(begin())` 逆序重建，4000 条记录时单次 516 ms | `AppState.hpp:get_read_history` `UIComponents.cpp` | 改为返回 `const&`；`ReadHistoryKeepsInsertionOrder` 锁顺序语义 |
| P2 | `read<T>(pos)` 允许传入绝对地址，但光标按 `cursor_pos` 相对推进，记录 `index` 却写 `pos` | `AppState.hpp:read/read_fixed_string/…` | 移除 `pos` 形参，读取入口统一到光标；`FailedReadsLeaveNoTrace` |
| P2 | `j 0xZZZ` 被 `stoul` 静默解析成 0 并跳到文件头 | `Command.cpp:j` | `CommandTest.JumpRejectsMalformedNumbers` |
| P2 | `r <未知类型>` 完全无提示（`ReaderFactory` 返回 false 但不写状态栏） | `AppState.cpp:ReaderFactory::read` | pty 实测 `r nosuch`→`Unknown type: nosuch` |
| P2 | 越界读取只显示笼统的 `Read failed.`，且各策略异常处理不一致 | `AppState.cpp`（统一 `catch`） | `CommandTest.ReadTypeAndFailureMessages` |
| P2 | 空回车报 `Unknown command: `（名字为空） | `EventHandlers.cpp:HandleCommands` | 空 `name` 不再分发 |
| P2 | `u` 无任何反馈；`char[大数]` 长度累加可回绕 | `Command.cpp:u` `Utils.hpp:parse_char_length` | `CommandTest.UndoReportsOffsetOrNothingToUndo` |
| P2 | 输入框占位符提示 `set be/le/pos <num>`——这三条命令都不存在 | `UIComponents.cpp:CommandLine` | 改为真实语法提示；README/cmd.md 同步 |

同时新增 `tests/test_command.cpp`（命令表行为）与事件层归属用例，单测 18 → 34 例全绿。

> 说明：`ReaderFactoryTest.UnknownType` 与 `LengthPrefixedStringReaders` 两个旧用例
> 原本断言的正是缺陷行为（未知类型必须静默、越界必须向外抛异常），本轮按新契约更新。

---

## 二、待办：正确性

| 级别 | 事项 | 说明 | 规模 |
|---|---|---|---|
| **P1** | **多余参数被静默忽略** | `r u16 3` 只读一个 u16，第 2 个参数（批量 count）被丢弃且**毫无提示**，用户会以为读了 3 个。README 与 cmd.md 已改为标注「部分实现 / 未实现」，但代码侧仍未处理：要么实现 count，要么明确报 `too many arguments` | M |
| P2 | 假定宿主为小端 | `peek<T>` 仅在 `!is_little_endian` 时反转字节，大端主机上语义相反。应先把数据归一化为小端再按模式处理（C++20 `std::endian::native`，或运行期探测） | S |
| P2 | 光标边界语义两套标准 | `read` 后光标可停在 `data.size()`（EOF 哨兵），但 `next()`/`next_line()` 用 `< data.size()`，无法用方向键到达同一位置；`j` 的绝对地址又强制 `< size`。建议在 design.md 里定死一套并统一实现 | S |
| P2 | 停在 EOF 哨兵时界面无提示 | 此时 hex 视图无高亮单元格、预览全 `N/A`，用户看不出「已越过最后一个字节」。状态栏应显式标记 EOF | S |
| P2 | `j +0x10` 被判为非法 | 相对跳转只吃十进制，绝对跳转才认 `0x`。建议相对量同样支持十六进制 | S |
| P3 | `string@` 前缀缺 `u64/i64`，`char[n]` 无上限校验 | 补齐八种前缀类型；对 `n` 设一个合理上限并给出可诊断错误 | S |

## 三、待办：性能

| 级别 | 事项 | 说明 | 规模 |
|---|---|---|---|
| P1 | 全文件一次性载入内存 | `Utils::read_binary_file` 把整个文件读进 `std::vector<uint8_t>`，内存 ≈ 文件大小且无上限告警。大文件（≥数百 MB）应先 `stat` 提示，长期方向是 mmap/按窗口读取，`peek`/`read` 改成「窗口内寻址」 | L |
| P1 | 每帧 O(历史长度) 全量重建 | `DataReadHistoryBar` 每条记录生成 3 个 `text()` 元素并 `fmt::format`，记录多时明显掉帧。需要虚拟滚动（只渲染可见行，FTXUI `Framework` + 滚动条状态），或至少把元素缓存到状态变更时 | M |
| P2 | `HexView` 每帧重建 `hex_view_h × 16` 个单元格 | 逐字节 `fmt::format("{:02X}")`。可按页缓存、仅在页/光标变化时重建 | M |
| P3 | `DataPreviewBar` 每帧新建 `vector<pair>` 并做 10 次 `peek` | 开销小（10 项），可在光标未变时复用 | S |

## 四、待办：功能缺口（cmd.md 已设计，未实现）

| 优先级 | 命令/能力 | 备注 | 规模 |
|---|---|---|---|
| 高 | `r <type> [count]` 批量读取 | 见「正确性」首条，当前是静默错误 | M |
| 高 | `help [cmd]` | 命令表已有 name→closure，可再加 usage 字符串；对可用性收益最大 | M |
| 高 | `/ <hex bytes>` 字节模式搜索 | 纯 `AppState` 层能力，结果写 `status_msg` 并可跳过去命中点 | M |
| 中 | `j <addr>` 之后支持标签/注释 | 给 `Record` 加 name，历史面板显示「section_header」之类 | M |
| 中 | `redo` | 目前 `undo` 单向丢弃记录；保留一个 redo 栈即可，同时让 `u` 语义可回退 | M |
| 中 | `.` 只重复导航，不重复上一条文本命令 | 提交信息是「support repeat cmd by .」，实际只覆盖方向键。`r u8` 后按 `.` 应继续读一个 u8（需在 `AppState` 存 `last_parsed` 并再分发；同时解决下面这条冲突） | S |
| 中 | `@ [lines]` 上下文窗口 | cmd.md 已设计 | S |
| 低 | `w <type> <value>` + `save [file]` | 需要引入脏标记、原始/修改双缓冲与写盘路径，属于一次小版本 | L |
| 低 | `config`（前缀类型、显示格式） | 需要配置载体 | M |
| 低 | 批处理 `-e 'j 0x100; r i32 3; save out.bin'` | 需要把命令链从 UI 解耦（当前 `HandleCommands` 直接依赖 FTXUI 事件与 `screen.Exit`） | L |
| 探索 | 结构化解析（一次读一组字段 / ELF、Java class 之类模板） | `ReaderStrategy` 抽象已可容纳复合读取，但 `Record` 需要支持嵌套与区间长度 | L |

## 五、待办：UI / 交互

| 级别 | 事项 | 说明 | 规模 |
|---|---|---|---|
| P2 | 未监听 `Event::Resize` | `hex_view_h` 固定 16，终端更矮时 hex 区被裁，页语义与可见内容脱节。应随高度更新（`workspace` 高度 = 终端高 − 状态栏 1 − 命令行 3 − 边框 2） | M |
| P2 | 面板宽度硬编码且互相冲突 | `MainUi` 给 hex/preview/history 设 150/35/35，两个侧栏内部又各自 `size(WIDTH, EQUAL, 30)`。实测 120 列终端下第三个面板被截断。建议 hex 用 `flex`、侧栏固定较窄宽度，窄终端降级为单栏 | M |
| P2 | `.` 被导航层占用，命令行无法输入点号 | 未来 `save out.bin` 之类带点参数会打不出来。建议 `.` 仅在输入框为空时生效（或改成 `Ctrl-R`） | S |
| P2 | 方向键被导航占用 | 输入较长命令时无法用 ←/→ 移动编辑光标、Home/End 不可用。可按「输入框非空时让给 Input」或引入显式模式切换 | M |
| P3 | 历史面板不可点击 | FTXUI 已开鼠标跟踪；点击某条记录跳到其 `index` 是低成本高收益交互 | S |
| P3 | `status_msg` 过长会挤掉 File 段 | 状态栏三段固定拼接，建议对消息做宽度自适应截断或换行 | S |
| P3 | 退出无二次确认、无脏数据保护 | 引入写功能后必须有 | S |

## 六、待办：工程性与文档

| 级别 | 事项 | 说明 | 规模 |
|---|---|---|---|
| P3 | `AppState.hpp` 是 god header（443 行、5 个概念） | 拆成 `Types.hpp`（TypeTraits/TypeFactory）、`Record.hpp`、`Readers.hpp`（策略+工厂），`AppState.hpp` 只留状态与读取原语 | M |
| P3 | 两套命令体系并存 | `Command`/`CommandType`（键盘导航，`apply_command` 的 `Jump`/`ReadByte` 分支是空壳）与 `ParsedCommand`/`CommandRegistry`（文本命令）。统一到 registry，导航作为普通命令注册 | M |
| P3 | 死代码 | `TypeFactory::type_map_` 只写不读（对外仅用静态 `getTypeShortName<T>`）；`Record::description()` 只被自己测试调用且输出不含数据值（`00000010:u16:`），易被误当展示 API；`TypedReader<T>::typeName_` 未使用 | S |
| P3 | 依赖管理不一致 | CLI11 以 10998 行单头 vendored 在 `include/`，其余走 `FetchContent`。统一后少一处手工升级点 | S |
| P3 | `build.sh` 使用 `git config --global` | 永久改写开发者机器的 git 配置（把 `https://github.com/` 换成 SSH insteadOf），且与 `.github/workflows/ci.yml` 里「Force HTTPS」步骤互相抵消。改用仓库局部配置或 `GIT_CONFIG_COUNT` 环境变量；`rm -rf build` 也应改成 `cmake -B` 自带清理 | S |
| P3 | `FETCHCONTENT_BASE_DIR = $HOME/_deps` | 在工程目录之外写入，且与本机其他 CMake 工程的 `_deps` 混在一起（当前确实混了别的项目）。建议默认 `build/_deps`，需要缓存时用环境变量显式指定 | S |
| P3 | `-Werror` + 第三方告警补丁脆弱 | 只对 `dom`/`screen` 关掉 `-Werror=pedantic`；升级 FTXUI 或换编译器版本会再爆。建议第三方 target 统一按 SYSTEM include / 关掉 warning 传播处理 | S |
| P3 | `BUILD_TESTING` 未被声明为 option | `if(BUILD_TESTING)` 之前没有 `include(CTest)`，因此默认关闭且不会出现在 `cmake-gui`/option 列表里——只传 `-DBUILD_TESTING=ON` 才构建测试，裸 `cmake -B build` 会静默跳过 | S |
| P3 | CI 覆盖不足 | 只有 `ubuntu-latest`，而主力开发环境是 macOS（arm64）；无 ccache/`_deps` 缓存，每次全量拉依赖；`build.sh`（唯一的「一键跑通」入口）本身不在 CI 里执行；测试产物与 `demo.gif` 缺失也无检查 | M |
| P3 | 无 `clang-format` / `.clang-tidy` | 注释中英混排、`using namespace ftxui;` 仍留在 `UIComponents.hpp` 与两个 `.cpp` 顶部；头文件包含顺序无约束。风格工具化后再统一改一次，避免每次 PR 顺手重排 | S |
| P3 | 测试盲区 | ① 没有 `MainUi` 的焦点/事件路由用例——正是本次 P0 的成因，建议用 `ScreenInteractive::FixedSize` + `Post`/`PostEvent` 驱动一轮键入并断言 `cmd` 内容与 `status_msg`；② `Utils.hpp` 完全没有单测（`format_any` 的类型分派、`parse_char_length` 的溢出与非数字分支）；③ `tests/data/*` 未被任何测试引用 | M |
| P3 | pty 端到端验证不可复用 | 本轮 P0 是靠临时手写的 python pty 驱动 + VT100 反解析定位的（配方记在 design.md §9），脚本未进仓库，终端尺寸与按键序列只存在于命令行里。建议落成 `tests/e2e/` 下的脚本用例（或用 CTest `PASS_FAIL_REGEX`），让「键入可见 → 回车生效 → 状态栏文案」成为 CI 断言，而不是每次重新发明 | M |
| P2 | 文档漂移（本轮已收拾一轮） | README 与 cmd.md 已按实际行为重写；仍缺 `demo.gif`，且 cmd.md 中「设计中」的命令需要随实现进度持续挪动状态标记，否则又会失真 | S |

---

## 七、建议的推进顺序

1. **先修「多余参数被静默忽略」**（`r u16 3`），它是当前唯一会给出错误结论的缺陷。
2. **`help` + `.` 重复上一条文本命令 + 让出点号**：三者都在命令层，一次做完，
   可发现性和重复劳动的收益立竿见影。
3. **布局自适应（Resize + 面板宽度）**：不解决它，任何 80 列终端用户都会看到残废界面。
4. **历史/Hex 面板的按页缓存 + 虚拟滚动**，随后再评估 mmap/大文件。
5. **`/` 搜索**：不依赖写路径，是最高价值的新增能力。
6. 以上落定后，再开 `w`/`save`（需要双缓冲与脏标记，属于独立里程碑）。
7. 工程性三项（god header 拆分、命令体系统一、格式化工具化）安排在功能间歇，
   避免与功能改动同批提交造成 diff 噪声。
