#include "BinaryEditor.hpp"
#include <filesystem>
#include <gtest/gtest.h>

class StringTest : public ::testing::Test {
protected:
  const std::string test_file = "string_test.bin";

  void SetUp() override {
    // 创建带有测试数据的文件
    std::ofstream f(test_file, std::ios::binary);

    // 定长字符串区域 (offset 0)
    const char fixed_str[10] = "fixed\x00str"; // 包含空字符
    f.write(fixed_str, 10);

    // 长度前缀字符串区域 (offset 10)
    const uint32_t len_prefix = 6; // "prefix"
    f.write(reinterpret_cast<const char *>(&len_prefix), 4);
    f.write("prefix", 6);
  }

  void TearDown() override { std::remove(test_file.c_str()); }
};

TEST_F(StringTest, FixedLengthNormal) {
  BinaryEditor editor(test_file);

  // 读取10字节定长字符串
  auto str = editor.read_fixed_string(10);
  EXPECT_EQ(str.size(), 10);
  EXPECT_EQ(str, std::string("fixed\0str", 10)); // 验证包含空字符

  // 验证读取指针位置
  EXPECT_EQ(editor.read_index(), 10);
}

TEST_F(StringTest, FixedLengthBoundary) {
  BinaryEditor editor(test_file);

  // 尝试读取超过文件大小的数据
  editor.set_read_index(15);
  EXPECT_THROW(editor.read_fixed_string(10), std::out_of_range);

  // 验证指针未改变
  EXPECT_EQ(editor.read_index(), 15);
}

TEST_F(StringTest, FixedLengthZero) {
  BinaryEditor editor(test_file);

  // 读取0字节
  auto str = editor.read_fixed_string(0);
  EXPECT_TRUE(str.empty());
}

TEST_F(StringTest, LengthPrefixNormal) {
  BinaryEditor editor(test_file);
  editor.set_read_index(10); // 定位到前缀区

  // 读取uint32_t前缀字符串
  auto str = editor.read_length_prefixed_string<uint32_t>();
  EXPECT_EQ(str, "prefix");
  EXPECT_EQ(editor.read_index(), 10 + 4 + 6); // 前缀4字节+内容6字节
}

TEST_F(StringTest, LengthPrefixOverflow) {
  // 创建特殊测试文件
  std::ofstream f("overflow.bin", std::ios::binary);
  const uint32_t invalid_len = 1000;
  f.write(reinterpret_cast<const char *>(&invalid_len), 4);
  f.close();

  BinaryEditor editor("overflow.bin");
  EXPECT_THROW(editor.read_length_prefixed_string<uint32_t>(),
               std::out_of_range);

  std::remove("overflow.bin");
}

TEST_F(StringTest, DifferentPrefixTypes) {
  // 测试uint8_t前缀
  std::ofstream f8("pre8.bin", std::ios::binary);
  const uint8_t len8 = 3;
  f8.write(reinterpret_cast<const char *>(&len8), 1);
  f8.write("abc", 3);
  f8.close();

  BinaryEditor ed8("pre8.bin");
  EXPECT_EQ(ed8.read_length_prefixed_string<uint8_t>(), "abc");
  std::remove("pre8.bin");
}

TEST_F(StringTest, UndoOperations) {
  BinaryEditor editor(test_file);

  // 读取定长字符串
  auto s1 = editor.read_fixed_string(10);
  // 读取带前缀字符串
  auto s2 = editor.read_length_prefixed_string<uint32_t>();
  std::vector<Record> records = editor.get_read_history();
  EXPECT_EQ(records.size(), 2);
  EXPECT_EQ(records[0].type_name, "string[10]");
  EXPECT_EQ(records[1].type_name, "u32@string");
  // 验证undo操作
  editor.undo(); // 撤销前缀字符串
  editor.undo(); // 撤销定长字符串
  EXPECT_EQ(editor.read_index(), 0);

  // 重新读取验证数据
  EXPECT_EQ(editor.read_fixed_string(10), s1);
}