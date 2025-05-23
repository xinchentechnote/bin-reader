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
    std::vector<uint8_t> data;  // 文件数据
    size_t cursor_pos = 0;      // 当前光标位置
    size_t bytes_per_line = 16; // 每行显示字节数
    size_t current_page = 0;    // 当前页
    std::string status_msg;     // 状态消息

    // 计算总页数
    [[nodiscard]] size_t total_pages(const int term_height) const
    {
        const size_t lines = (data.size() + bytes_per_line - 1) / bytes_per_line;
        return lines / std::max(1, term_height - 4);
    }
};