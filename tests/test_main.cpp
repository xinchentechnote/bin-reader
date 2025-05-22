#include <gtest/gtest.h>
#include "../src/BinaryEditor.hpp"
#include <fstream>
#include <cstdio>

class BinaryEditorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 创建测试文件
        std::ofstream test_file("test.bin", std::ios::binary);
        const uint32_t data = 0x12345678;
        test_file.write(reinterpret_cast<const char *>(&data), sizeof(data));
    }

    void TearDown() override
    {
        // 清理测试文件
        std::remove("test.bin");
    }
};

TEST_F(BinaryEditorTest, BasicRead_SingleUint32)
{
    BinaryEditor editor("test.bin");
    auto result = editor.read<uint32_t>();
    EXPECT_EQ(result[0], 0x12345678);
}

TEST_F(BinaryEditorTest, BasicRead_MultipleUint8)
{
    BinaryEditor editor("test.bin");
    auto result = editor.read<uint8_t>(4);
    EXPECT_EQ(result[0], 0x78);
    EXPECT_EQ(result[1], 0x56);
    EXPECT_EQ(result[2], 0x34);
    EXPECT_EQ(result[3], 0x12);
}

TEST_F(BinaryEditorTest, UndoReadOperation) 
{
    // Create initial file with known data
    const std::string filename = "test.bin";
    {
        std::ofstream test_file(filename, std::ios::binary);
        const uint32_t test_data = 0x12345678;
        test_file.write(reinterpret_cast<const char*>(&test_data), sizeof(test_data));
    }

    BinaryEditor editor(filename);
    
    // First read - should be 0x78
    auto data = editor.read<uint8_t>();
    EXPECT_EQ(data[0], 0x78);
    
    // Second read - should be 0x56
    data = editor.read<uint8_t>();
    EXPECT_EQ(data[0], 0x56);
    
    // Third read - should be 0x34
    data = editor.read<uint8_t>();
    EXPECT_EQ(data[0], 0x34);
    
    // Fourth read - should be 0x12
    data = editor.read<uint8_t>();
    EXPECT_EQ(data[0], 0x12);
    
    // Undo last two reads (positions 2 and 3)
    editor.undo();  // Undo position 3
    editor.undo();  // Undo position 2
    
    // Next read should be from position 2 again (0x56)
    data = editor.read<uint8_t>();
    EXPECT_EQ(data[0], 0x34);  // Same as third read
}