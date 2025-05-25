#include "AppState.hpp"
#include <gtest/gtest.h>

// --------- AppState 基础测试 ---------
TEST(AppStateTest, PeekAndRead_uint16) {
  AppState state;
  // 小端序：0x11,0x22 -> uint16_t = 0x2211
  state.data = {0x11, 0x22, 0x33, 0x44};
  state.is_little_endian = true;

  // peek<uint16_t>
  uint16_t peeked = state.peek<uint16_t>(0);
  EXPECT_EQ(peeked, 0x2211);

  // peek 越界测试
  EXPECT_THROW(state.peek<uint32_t>(2), std::out_of_range);

  // read<uint16_t>，并验证 cursor_pos、read_history
  uint16_t readVal = state.read<uint16_t>(0);
  EXPECT_EQ(readVal, 0x2211);
  EXPECT_EQ(state.cursor_pos, sizeof(uint16_t));
  // read_history 应该有 1 条记录，记录 index=0，data=0x2211
  {
    auto hist = state.get_read_history();
    ASSERT_EQ(hist.size(), 1u);
    EXPECT_EQ(hist[0].index, 0u);
    EXPECT_EQ(std::any_cast<uint16_t>(hist[0].data), (uint16_t)0x2211);
  }

  // 再次 read，cursor 在 2 位置
  uint16_t read2 = state.read<uint16_t>(state.cursor_pos);
  EXPECT_EQ(read2, 0x4433); // 原数组 {0x33,0x44}
  EXPECT_EQ(state.cursor_pos, 4u);
  EXPECT_EQ(state.get_read_history().size(), 2u);
}

TEST(AppStateTest, ReadFixedStringAndUndo) {
  AppState state;
  state.data = {'H', 'e', 'l', 'l', 'o', 'X'};

  // read_fixed_string(0,5)
  std::string str = state.read_fixed_string(0, 5);
  EXPECT_EQ(str, "Hello");
  EXPECT_EQ(state.cursor_pos, 5u);
  // read_history 只有一条，index=0，data="Hello"
  {
    auto hist = state.get_read_history();
    ASSERT_EQ(hist.size(), 1u);
    EXPECT_EQ(hist[0].index, 0u);
    EXPECT_EQ(std::any_cast<std::string>(hist[0].data), "Hello");
  }

  // undo：将 cursor_pos 回退到 0，history 为空
  state.undo();
  EXPECT_EQ(state.cursor_pos, 0u);
  EXPECT_TRUE(state.get_read_history().empty());

  // 越界测试
  EXPECT_THROW(state.read_fixed_string(2, 10), std::out_of_range);
}

TEST(AppStateTest, ReadLengthPrefixedString) {
  AppState state;
  // 构造：长度前缀 uint8_t=3，后跟 'A','B','C'
  state.data = {0x03, 'A', 'B', 'C', 'X', 'Y'};
  state.is_little_endian = true;
  state.cursor_pos = 0;

  // 读 u8 前缀 + 固定字符串
  std::string lp = state.read_length_prefixed_string<uint8_t>(0);
  EXPECT_EQ(lp, "ABC");
  // cursor_pos 前进 1+3=4
  EXPECT_EQ(state.cursor_pos, 4u);

  // read_history 只保留合并后的一个 Record (index=0, data="ABC")
  {
    auto hist = state.get_read_history();
    ASSERT_EQ(hist.size(), 1u);
    EXPECT_EQ(hist[0].index, 0u);
    EXPECT_EQ(std::any_cast<std::string>(hist[0].data), "ABC");
  }

  // 越界恢复测试：前缀值超过剩余字节长度
  state.data = {0x05, 'A', 'B'}; // len=5，但剩余只有 2 字节
  state.cursor_pos = 0;
  EXPECT_THROW(state.read_length_prefixed_string<uint8_t>(0),
               std::out_of_range);
  // 异常后 cursor_pos 应恢复到 start_pos（0）
  EXPECT_EQ(state.cursor_pos, 0u);
}

TEST(AppStateTest, CursorMovementAndPages) {
  AppState state;
  // 填充 100 个字节
  state.data.resize(100, 0xFF);
  state.bytes_per_line = 10;
  state.hex_view_h = 2;
  // total_lines = ceil(100/10) = 10 行；lines_per_page =
  // max(1,2)=2；total_pages = ceil(10/2)=5
  EXPECT_EQ(state.total_pages(), 5u);

  // 一开始 cursor_pos=0, current_page=0
  EXPECT_EQ(state.cursor_pos, 0u);
  EXPECT_EQ(state.current_page, 0u);

  // move 越界：move 200 bytes 应返回 false
  EXPECT_FALSE(state.move(200));
  EXPECT_EQ(state.cursor_pos, 0u);

  // move 到 15
  EXPECT_TRUE(state.move(15));
  EXPECT_EQ(state.cursor_pos, 15u);
  // 15 / (10*2) = 15/20 = 0 ⇒ current_page 仍为 0
  EXPECT_EQ(state.current_page, 0u);

  // 再 move 10 => cursor_pos = 25, 25/20 = 1 => current_page=1
  EXPECT_TRUE(state.move(10));
  EXPECT_EQ(state.cursor_pos, 25u);
  EXPECT_EQ(state.current_page, 1u);

  // pre()：前移 1
  EXPECT_TRUE(state.pre());
  EXPECT_EQ(state.cursor_pos, 24u);
  // pre_line(): 减少一行（10 字节）
  EXPECT_TRUE(state.pre_line());
  EXPECT_EQ(state.cursor_pos, 14u);
  // next_line(): 增加一行到 24
  EXPECT_TRUE(state.next_line());
  EXPECT_EQ(state.cursor_pos, 24u);

  // next(): 正常前移 1
  EXPECT_TRUE(state.next());
  EXPECT_EQ(state.cursor_pos, 25u);

  // pre_page：当前 page=1 => 可以前一页
  EXPECT_TRUE(state.pre_page());
  EXPECT_EQ(state.current_page, 0u);
  EXPECT_EQ(state.cursor_pos, 0u); // 回到 page0 的起始位置

  // next_page：page 0 -> page 1
  EXPECT_TRUE(state.next_page());
  EXPECT_EQ(state.current_page, 1u);
  EXPECT_EQ(state.cursor_pos, 1u * 10 * 2); // 20

  // next_page 到最后一页
  EXPECT_TRUE(state.next_page());  // page2
  EXPECT_TRUE(state.next_page());  // page3
  EXPECT_TRUE(state.next_page());  // page4
  EXPECT_FALSE(state.next_page()); // 已到最后一页，无法再前进
  EXPECT_EQ(state.current_page, 4u);
}

TEST(AppStateTest, SetCursorPosBoundaries) {
  AppState state;
  state.data.resize(10, 0x00);

  // new_pos 必须在 (0, data.size()-1) 之间，1~8 之间有效
  EXPECT_TRUE(state.set_cursor_pos(0));
  EXPECT_TRUE(state.set_cursor_pos(1));
  EXPECT_EQ(state.cursor_pos, 1u);
  EXPECT_TRUE(state.set_cursor_pos(8));
  EXPECT_EQ(state.cursor_pos, 8u);
  EXPECT_TRUE(state.set_cursor_pos(9));
  EXPECT_EQ(state.cursor_pos, 9u); // 保持不变
}

// --------- ReaderFactory & ReaderStrategy 测试 ---------

TEST(ReaderFactoryTest, TypedReader_uint32) {
  AppState state;
  // 构造一个 uint32_t 值：0x04030201
  state.data = {0x01, 0x02, 0x03, 0x04, 0xAA};
  state.is_little_endian = true;
  state.cursor_pos = 0;

  bool ok = ReaderFactory::instance().read(state, "u32");
  EXPECT_TRUE(ok);
  // read<uint32_t> 会读取 4 字节
  EXPECT_EQ(state.cursor_pos, 4u);
  // status_msg 至少要包含 "Read u32:" 和 "@ 0x0"
  {
    auto pos1 = state.status_msg.find("Read u32:");
    auto pos2 = state.status_msg.find("@ 0x0");
    EXPECT_NE(pos1, std::string::npos);
    EXPECT_NE(pos2, std::string::npos);
  }
}

TEST(ReaderFactoryTest, TypedReader_float_bigEndian) {
  AppState state;
  // 构造一个 float 值：1.0f（IEEE754 小端：00 00 80 3F），我们写入大端顺序
  state.is_little_endian = false;
  state.data = {0x3F, 0x80, 0x00, 0x00};
  state.cursor_pos = 0;

  bool ok = ReaderFactory::instance().read(state, "f32");
  EXPECT_TRUE(ok);
  EXPECT_EQ(state.cursor_pos, 4u);
  // status_msg 至少要包含 "Read f32:" 和 "@ 0x0"
  {
    auto pos1 = state.status_msg.find("Read f32:");
    auto pos2 = state.status_msg.find("@ 0x0");
    EXPECT_NE(pos1, std::string::npos);
    EXPECT_NE(pos2, std::string::npos);
  }
}

TEST(ReaderFactoryTest, FixStringReader_SuccessAndFail) {
  {
    AppState state;
    // 构造 5 字节字符串 "HELLO"
    state.data = {'H', 'E', 'L', 'L', 'O', 'Z'};
    state.cursor_pos = 0;
    bool ok1 = ReaderFactory::instance().read(state, "char[5]");
    EXPECT_TRUE(ok1);
    EXPECT_EQ(state.cursor_pos, 5u);
    // status_msg 至少要包含 "Read char[5]:" 和 "HELLO" 和 "@ 0x0"
    {
      auto &msg = state.status_msg;
      EXPECT_NE(msg.find("Read char[5]:"), std::string::npos);
      EXPECT_NE(msg.find("HELLO"), std::string::npos);
      EXPECT_NE(msg.find("@ 0x0"), std::string::npos);
    }
  }
  {
    // 失败测试：只有 3 字节，但尝试读取 5 字节
    AppState state2;
    state2.data = {'A', 'B', 'C'};
    state2.cursor_pos = 0;
    bool ok2 = ReaderFactory::instance().read(state2, "char[5]");
    EXPECT_FALSE(ok2);
    // status_msg 至少要包含 "char[5] read failed"
    EXPECT_NE(state2.status_msg.find("char[5] read failed"), std::string::npos);
  }
}

TEST(ReaderFactoryTest, LengthPrefixedStringReaders) {
  // 1) string@u8
  {
    AppState state;
    state.data = {0x03, 'X', 'Y', 'Z'};
    state.cursor_pos = 0;
    bool ok = ReaderFactory::instance().read(state, "string@u8");
    EXPECT_TRUE(ok);
    EXPECT_EQ(state.cursor_pos, 4u);
    // status_msg 至少要包含 "Read lpstring:" 和 "@ 0x0"
    {
      auto &msg = state.status_msg;
      EXPECT_NE(msg.find("Read string@u8:"), std::string::npos);
      EXPECT_NE(msg.find("@ 0x0"), std::string::npos);
    }
  }
  // 2) string@u16
  {
    AppState state;
    // u16 length = 2 (little endian 表示为 0x02,0x00)，后跟 “OK”
    state.data = {0x02, 0x00, 'O', 'K'};
    state.is_little_endian = true;
    state.cursor_pos = 0;
    bool ok = ReaderFactory::instance().read(state, "string@u16");
    EXPECT_TRUE(ok);
    EXPECT_EQ(state.cursor_pos, 4u);
    {
      auto &msg = state.status_msg;
      EXPECT_NE(msg.find("Read string@u16:"), std::string::npos);
      EXPECT_NE(msg.find("@ 0x0"), std::string::npos);
    }
  }
  // 3) string@i8 越界测试
  {
    AppState state;
    state.data = {0x05, 'A', 'B'}; // 长度 5，但只有 2 字节
    state.cursor_pos = 0;
    EXPECT_THROW(ReaderFactory::instance().read(state, "string@i8"),
                 std::out_of_range);
    // cursor_pos 应恢复到 0
    EXPECT_EQ(state.cursor_pos, 0u);
  }
}

TEST(ReaderFactoryTest, UnknownType) {
  AppState state;
  state.data = {0x00, 0x00, 0x00, 0x00};
  state.cursor_pos = 0;
  // 调用一个不存在的类型，应返回 false，不修改状态
  bool ok = ReaderFactory::instance().read(state, "not_exist");
  EXPECT_FALSE(ok);
  EXPECT_EQ(state.cursor_pos, 0u);
  EXPECT_TRUE(state.status_msg.empty());
}

// --------- Record.description() 测试 ---------

TEST(RecordTest, DescriptionFormatting) {
  Record r1(0x10, (uint16_t)0x1234);
  r1.type_name = "u16";
  // description: index:0x00000010; type_name:u16; ... 这里只验证前缀部分包含
  // "00000010:u16:"
  std::string desc = r1.description();
  EXPECT_NE(desc.find("00000010:u16:"), std::string::npos);
}
