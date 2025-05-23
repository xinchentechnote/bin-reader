#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>
#include <vector>
#include <string>

using namespace ftxui;
// ========== 数据模型 ==========
struct AppState
{
    std::vector<uint8_t> data;    // 文件数据
    size_t cursor_pos = 0;        // 当前光标位置
    size_t bytes_per_line = 16;   // 每行显示字节数
    size_t current_page = 0;      // 当前页
    std::string status_msg;       // 状态消息
    bool is_little_endian = true; // 默认小端序

    // 计算总页数
    [[nodiscard]] size_t total_pages(const int term_height) const
    {
        const size_t lines = (data.size() + bytes_per_line - 1) / bytes_per_line;
        return lines / std::max(1, term_height - 4);
    }

    // 通用数据读取函数
    template <typename T>
    std::string read_value(size_t pos) const
    {
        if (pos + sizeof(T) > data.size())
            return "N/A";

        T value;
        memcpy(&value, &data[pos], sizeof(T));

        // 需要处理字节序转换
        if (!is_little_endian)
        {
            reverse_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(T));
        }

        return format_value(value);
    }

private:
    // 字节序反转
    void reverse_bytes(uint8_t *bytes, size_t size) const
    {
        for (size_t i = 0; i < size / 2; ++i)
        {
            std::swap(bytes[i], bytes[size - 1 - i]);
        }
    }

    // 数值格式化
    template <typename T>
    std::string format_value(T value) const
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return fmt::format("{:.3f}", value);
        }
        else if constexpr (std::is_signed_v<T>)
        {
            return fmt::format("{}", value);
        }
        else
        {
            return fmt::format("{}", value);
        }
    }
};