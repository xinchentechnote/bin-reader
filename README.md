# Bin-Reader

![Demo](demo.gif) *(todo)*

一个基于 **C++** 和 **FTXUI** 的交互式二进制文件解析工具，支持动态读取、回滚操作和友好的终端界面。

## 功能特性

- **交互式 TUI**: 使用 [FTXUI](https://github.com/ArthurSonzogni/FTXUI) 构建直观的命令行界面。
- **基本数据类型**: i8 u8 i16 u16 i32 u32 i64 u64 f32 f64
- **字符串类型**: char[n] u8@string@utf8 
- **多数据类型解析**:
  - `r i32 [N]`: 读取 `int32` 类型（默认 `N=1`）。
  - `r i16 [N]`: 读取 `int16` 类型。
  - `r u16 [N]`: 读取 `uint16` 类型。
  - `r u16 [N] u32 [N]`: 读取 `uint16` 类型。
  - `r char[10]`: 读取定长字符串。
  - `r u8@string@utf8`: 读取长度前缀为u8的UTF-8变长字符串。
- **历史回滚**: 输入 `u` 撤销上一步操作。
- **实时信息**: 输入 `info` 显示当前文件偏移量和大小。
- **实时信息**: 输入 `list` 显示已读数据
- **实时信息**: 输入 `offset` 修改offset
- **错误处理**: 检测越界访问和无效命令。

## 安装

### 依赖项

- C++17 编译器（如 `g++` 或 `clang++`）
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) 库
- CMake（推荐）

### 步骤

1. **克隆仓库**:
   ```bash
   git clone https://github.com/xinchentechnote/bin-reader.git
   cd bin-reader
   ```

2. **安装 FTXUI**:
   ```bash
   git clone https://github.com/ArthurSonzogni/FTXUI.git
   cd FTXUI
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

3. **运行**:
   ```bash
   ./bin-reader /path/to/your/file.bin
   ```

## 使用示例

### 基本操作

```bash
加载文件
./bin-reader example.bin

读取两个 int32
> r i32 2
int32: 1234
int32: -5678

回滚到上一步
> u
Undo to offset: 0

读取三个 uint16
> r ui16 3
uint16: 42
uint16: 65535
uint16: 100

查看当前状态
> info
Offset: 6/1024
```


## 协议

MIT License. 详情见 [LICENSE](LICENSE).
