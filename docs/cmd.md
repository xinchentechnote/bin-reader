# 命令语法设计与实现状态

> 本文件是**设计稿**，记录命令体系的意图与目标语法；实际可用命令以 [README](../README.md) 的命令表为准。
> 每条命令标注状态：
> - **已实现** —— 行为与本文件描述一致
> - **部分实现** —— 语法或参数有出入，按「实际行为」段落使用
> - **设计中** —— 尚未实现，输入会得到 `Unknown command: <name>`

状态速览（2026-10）：

| 命令 | 设计语法 | 状态 | 实际行为 / 差异 |
| --- | --- | --- | --- |
| `r` | `r <type> [count] [@addr]` | 部分实现 | 只支持 `r <type>`，从当前光标读一个；`[count]`、`[@addr]` 被静默忽略 |
| `j` | `j <addr>` | 已实现 | 额外支持 `j +N` / `j -N` 相对跳转，以及无参 `j` 显示当前位置 |
| `u` | 未在初版设计中列出 | 已实现 | 撤销上一条读取，光标回到该记录起始偏移 |
| `be` / `le` | 设计为 `config endian …` | 已实现 | 直接切换字节序，无需 `config` |
| `q` / `quit` | `exit` / `quit` | 已实现 | `exit` 未注册；另有 `Esc` 键退出 |
| `.` | 未在初版设计中列出 | 已实现 | 重复上一条**导航**动作（非文本命令） |
| 方向键 / `PageUp` / `PageDown` | 未在初版设计中列出 | 已实现 | 字节、行、页三级移动 |
| `@` | `@ [lines]` | 设计中 | 上下文已由 hex 区常驻显示替代 |
| `/` | `/ <hex-byte>…` | 设计中 | — |
| `w` | `w <type> <value> [@addr]` | 设计中 | 工具目前只读，不修改内存 |
| `help` | `help [command]` | 设计中 | — |
| `save` | `save [filename]` | 设计中 | — |
| `config` | `config <key> <value>` | 设计中 | 仅 `pstr_prefix` 通过 `string@<type>` 内联表达，`display` 未实现 |
| 批处理 | `binary-reader demo.bin -e '…'` | 设计中 | CLI 目前只接受 `-f/--file`（必填、须存在） |

## 读取命令 `r`

**部分实现。**

实际语法：

```
r <type>
```

类型标识符（与 `ReaderFactory` 注册名一一对应）：

```
  i8  → int8_t      u8  → uint8_t
  i16 → int16_t     u16 → uint16_t
  i32 → int32_t     u32 → uint32_t
  i64 → int64_t     u64 → uint64_t
  f32 → float       f64 → double
  char[n]                → 定长字符串（n 字节，读入内存历史时以 {"…"} 呈现）
  string@u8|u16|u32       → 长度前缀字符串
  string@i8|i16|i32       → 有符号前缀，前缀为负时报错
```

与初版设计的差异：

- 设计中写作 `str[N]` / `pstr`，实现为 `char[n]` / `string@<prefix-type>`；前缀类型写在类型名里，而不是由 `config pstr_prefix` 设置。
- 设计中 `pstr` 支持 u64/i64 前缀，实现未注册 `string@u64` / `string@i64`（`std::any` 记录长度会失真，故未开放）。
- `[count]` 批量读取未实现。`r u16 3` 只读一个 u16，`3` 被忽略 —— 这是当前最值得修的文档/行为不一致。
- `[@addr]` 未实现，需先 `j <addr>`。

成功输出（状态栏）：`Read <type>: <value> @ 0x<起始偏移>`。浮点保留 3 位小数，其余按十进制显示。
失败输出：`Unknown type: <type>` 或 `<type> read failed: <原因>`；失败不推进光标、不写入历史。

示例（`tests/data/msg.bin`，13 字节）：

```
> be
Big-endian mode
> r u16
Read u16: 1 @ 0x0
> r u16
Read u16: 9 @ 0x2
> r char[9]
Read char[9]: {"a":123} @ 0x4
```

## 跳转命令 `j`

**已实现**，且比初版设计多出相对跳转。

```
j <address>   绝对跳转，十六进制需 0x/0X 前缀，其余按十进制解析
j +<n>        向后移动 n 字节
j -<n>        向前移动 n 字节
j             无参数：显示当前位置
```

边界：绝对地址要求 `0 <= addr < data.size()`；相对跳转允许落到 `data.size()`（EOF 哨兵），落在范围外返回 `Invalid relative position!`。

示例：

```
> j 0
Position set to 0x0
> j 0x4
Position set to 0x4
> j +4
Jumped 4 bytes to 0x8
> j -2
Jumped -2 bytes to 0x6
```

数字解析失败统一为 `Invalid jump value.`；越界区分 `Invalid hex position!` 与 `Invalid absolute position!`。

## 撤销命令 `u`

**已实现**（初版设计未列出，随历史记录一起加入）。

弹出最近一条读取记录，光标回到该记录的 `index`（起始偏移）。历史记录遵循「一次读取 = 一条记录」，长度前缀字符串的前缀与内容合并为一条，因此 `u` 一步回到字符串起始处而非前缀处。

无可撤销记录时：`Nothing to undo.`

## 字节序命令 `be` / `le`

**已实现**。等价于设计中 `config endian` 的两个取值，默认小端。

`peek<T>` 在读取时按目标字节序重排字节，因此切序后重读同一偏移即可得到另一端的解释。

## 查看上下文命令 `@`

**设计中**。当前 hex 区常驻显示当前页（每页 `hex_view_h` 行 × `bytes_per_line` 列），且状态栏持续显示 `Pos` / `Page`，故未单独实现 `@`。

若仍要实现，建议输出形态保持设计原样：

```
[0x00000100]  7F 45 4C 46 02 01 01 00  .ELF....
```

## 搜索命令 `/`

**设计中**。

```
/ <hex-byte1> <hex-byte2> ...
```

示例（设计目标）：

```
> / 7F 45 4C 46
Found at 0x00000100
```

## 写入命令 `w`

**设计中**。当前工具只读，内存中的 `data` 不会被修改，也没有落盘路径。

```
w <type> <value> [@address]
```

设计示例：

```
> w u16 0x1234
Write OK
> w str[5] "HELLO" @0x300
Write OK
```

## `help` / `save` / `config`

**设计中**。

- `help [command]`：设计目标是从命令注册表自动生成可发现性文档。
- `save [filename]`：依赖 `w`。
- `config pstr_prefix u16` / `config display hex`：长度前缀类型已由 `string@u16` 内联表达；显示格式切换仍空缺。

## 交互界面示例（设计稿）

以下保留初版设计的期望形态，其中 `str[8]`、`/`、`w`、`save`、`exit` 均未实现：

```bash
Welcome to BinaryReader v1.0
File: demo.bin (Size: 1.44MB)
Current Address: 0x00000000
r i32 2 @0x100

[0x00000100] 0x7F 0x45 0x4C 0x02 → int32[2]: [0x7F454C02, 0x...]
j 0x200

Jumped to 0x00000200
r str[8]

[0x00000200] 0x48 0x65 0x6C 0x6C → "HelloWor"
/ 57 6F 72 64

Found "World" at 0x00000205
w u8 0x21 @0x205

Write OK
save modified.bin

Saved 1.44MB to modified.bin
exit
```

## 批处理模式

**设计中**。当前 `main` 只解析 `-f/--file`，非交互运行没有落点。

```
$ bin-reader demo.bin -e 'j 0x100; r i32 3; save out.bin'
```

## 设计原则

以下原则当前已成立，后续改动需保持：

- **一致性**：命令结构遵循 `<verb> <type> <params>`。
- **二进制友好**：地址使用十六进制（`0x` 前缀），数值同时接受十进制。
- **状态可见**：`Pos` / `Page` / `Endian` / 文件名常驻状态栏；重要操作后有确认文案。
- **失败无副作用**：任何失败都不污染光标与历史记录。

以下原则尚待补齐：

- **可发现性**：`help` 未实现；命令注册表与 `ParsedCommand` 两套体系并存，自动生成文档前需先统一。
