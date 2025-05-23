命令格式                  说明 示例

r <type> [N] @[addr] 读取数据（支持批量、指定地址） r i32 3 @0x100
j <addr> 跳转到指定地址 j 0x200
@ 显示当前地址和附近数据 @
/ <hex-pattern> 搜索字节模式 / 01 23 45
w <type> <value> 写入数据 w u16 0xABCD
help [command] 查看帮助 help r
save [filename] 保存修改 save backup.bin
exit / quit 退出程序 exit

详细命令设计
读取命令 r

语法：  
r <type>[N] [count] [@address]

参数说明：
<type>：数据类型标识符

  
  i8  → int8_t      u8  → uint8_t
  i16 → int16_t     u16 → uint16_t
  i32 → int32_t     u32 → uint32_t
  f32 → float       f64 → double
  str[N] → 定长字符串（N字节）
  pstr → 长度前缀字符串（前缀类型通过 `config` 设置）
  
[count]：读取数量（默认1）

[@address]：十六进制地址（默认当前地址）

示例：
读取 3 个 uint32 数据（当前地址）
r u32 3

从 0x100 地址读取 10 字节的字符串
r str[10] @0x100

读取长度前缀字符串（前缀为 u16）
r pstr @0x200

跳转命令 j

语法：  
j <address>

功能：
将当前指针跳转到指定地址

支持十六进制（0x 前缀）和十进制

示例：
跳转到文件起始位置
j 0

跳转到 0x1FF 地址
j 0x1FF

查看上下文命令 @

语法：  
@ [lines]

输出示例：
@

[0x00000100]  7F 45 4C 46 02 01 01 00  .ELF....
[0x00000108]  00 00 00 00 00 00 00 00  ........

搜索命令 /

语法：  
/ <hex-byte1> <hex-byte2> ...

示例：
搜索 ELF 文件头
/ 7F 45 4C 46

Found at 0x00000100

写入命令 w

语法：  
w <type> <value> [@address]

示例：
写入 0x1234 到当前地址（u16 类型）
w u16 0x1234

在 0x300 处写入字符串 "HELLO"
w str[5] "HELLO" @0x300

交互界面示例
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
高级功能扩展

配置命令 config
设置字符串前缀长度类型（u8/u16/u32）
config pstr_prefix u16

设置默认显示格式（hex/dec/ascii）
config display hex

批处理模式
$ binary-reader demo.bin -e 'j 0x100; r i32 3; save out.bin'

设计原则
一致性：命令结构遵循 <verb> <type> <params> 模式

可发现性：help 命令显示完整文档

二进制友好：

地址使用十六进制

数据支持多格式输入（0x 前缀、十进制、字符串）
状态可见：

始终显示当前地址

重要操作后有确认提示

此优化方案在保持简洁性的同时，提供了强大的二进制文件操作能力，适用于逆向工程、文件格式分析等多种场景。