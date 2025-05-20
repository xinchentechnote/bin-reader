#include <catch2/catch_test_macros.hpp>
#include "../src/BinaryEditor.hpp"
#include <cstdio>

TEST_CASE("Basic Read Operations") {
    // 创建测试文件
    {
        std::ofstream test_file("test.bin", std::ios::binary);
        const int32_t data = 0x12345678;
        test_file.write(reinterpret_cast<const char*>(&data), sizeof(data));
    }

    BinaryEditor editor("test.bin");
    
    SECTION("Read single int32") {
        auto result = editor.read<int32_t>();
        REQUIRE(result[0] == 0x12345678);
    }

    SECTION("Read multiple uint8") {
        auto result = editor.read<uint8_t>(4);
        REQUIRE(result[0] == 0x78);
        REQUIRE(result[1] == 0x56);
        REQUIRE(result[2] == 0x34);
        REQUIRE(result[3] == 0x12);
    }

    // 清理测试文件
    std::remove("test.bin");
}

TEST_CASE("Write and Undo Operations") {
    BinaryEditor editor("test.bin");
    
    SECTION("Write and verify") {
        editor.write<int32_t>({0xDEADBEEF});
        REQUIRE(editor.read<int32_t>()[0] == 0xDEADBEEF);
    }

    SECTION("Undo write operation") {
        editor.undo();
        REQUIRE(editor.read<int32_t>()[0] == 0x12345678);
    }
}