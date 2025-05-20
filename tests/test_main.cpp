#include <gtest/gtest.h>
#include "../src/BinaryEditor.hpp"
#include <fstream>
#include <cstdio>

class BinaryEditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试文件
        std::ofstream test_file("test.bin", std::ios::binary);
        const uint32_t data = 0x12345678;
        test_file.write(reinterpret_cast<const char*>(&data), sizeof(data));
    }

    void TearDown() override {
        // 清理测试文件
        std::remove("test.bin");
    }
};

TEST_F(BinaryEditorTest, BasicRead_SingleUint32) {
    BinaryEditor editor("test.bin");
    auto result = editor.read<uint32_t>();
    EXPECT_EQ(result[0], 0x12345678);
}

TEST_F(BinaryEditorTest, BasicRead_MultipleUint8) {
    BinaryEditor editor("test.bin");
    auto result = editor.read<uint8_t>(4);
    EXPECT_EQ(result[0], 0x78);
    EXPECT_EQ(result[1], 0x56);
    EXPECT_EQ(result[2], 0x34);
    EXPECT_EQ(result[3], 0x12);
}

TEST_F(BinaryEditorTest, WriteAndVerify) {
    // 覆盖初始文件
    {
        std::ofstream test_file("test.bin", std::ios::binary);
        test_file.write("\x78\x56\x34\x12", 4);  // 0x12345678
    }
    
    BinaryEditor editor("test.bin");
    editor.write<uint32_t>({0xDEADBEEF});
    auto data = editor.read<uint32_t>();
    EXPECT_EQ(data[0], 0xDEADBEEF);
}

TEST_F(BinaryEditorTest, UndoWriteOperation) {
    // 覆盖初始文件
    {
        std::ofstream test_file("test.bin", std::ios::binary);
        test_file.write("\x78\x56\x34\x12", 4);  // 0x12345678
    }
    
    BinaryEditor editor("test.bin");
    editor.write<uint32_t>({0xDEADBEEF});
    editor.undo();
    auto data = editor.read<uint32_t>();
    EXPECT_EQ(data[0], 0x12345678);
}