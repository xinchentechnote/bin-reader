#pragma once
#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "Utils.hpp"

using namespace ftxui;
// ========== UI组件 ==========
namespace UIComponents {
inline Component HexView(AppState &state) {
  return Renderer([&] {
           const size_t content_height = state.hex_view_h;
           const size_t start_line = state.current_page * content_height;
           Elements lines;
           for (size_t i = 0; i < content_height; ++i) {
             const size_t addr = (start_line + i) * state.bytes_per_line;
             if (addr >= state.data.size())
               break;

             Elements hex, ascii;
             for (size_t j = 0; j < state.bytes_per_line; ++j) {
               const size_t pos = addr + j;
               const bool is_active = (pos == state.cursor_pos);

               if (pos >= state.data.size()) {
                 hex.push_back(text("   "));
                 ascii.push_back(text(" "));
                 continue;
               }

               const uint8_t byte = state.data[pos];
               hex.push_back(text(fmt::format("{:02X} ", byte)) |
                             (is_active ? inverted : nothing));
               ascii.push_back(
                   text(std::string(
                       1, (byte >= 0x20 && byte <= 0x7E) ? char(byte) : '.')) |
                   color(Color::Yellow) | (is_active ? inverted : nothing));
             }

             lines.push_back(hbox(
                 {text(fmt::format("{:08x}: ", addr)) | color(Color::Blue),
                  hbox(hex), text("│") | color(Color::GrayDark), hbox(ascii)}));
           }

           return vbox(lines);
         }) |
         border;
}

inline Component StatusBar(AppState &state) {
  return Renderer([&] {
    return hbox({
        text(fmt::format(" Pos: 0x{:08x} ", state.cursor_pos)) |
            bgcolor(Color::DarkBlue),
        text(fmt::format(" Page: {}/{} ", state.current_page + 1,
                         state.total_pages())) |
            bgcolor(Color::DarkGreen),
        text(fmt::format(" {} ", state.status_msg)) | bgcolor(Color::DarkRed) |
            flex,
    });
  });
}

inline Component DataPreviewBar(AppState &state) {
  return Renderer([&] {
    size_t pos = state.cursor_pos;
    auto endian_str = state.is_little_endian ? "LE" : "BE";
    return vbox({
               hbox({text("Endian:") | bold | color(Color::GrayDark),
                     text(endian_str)}),
               hbox({text(" i8: ") | color(Color::Green) | flex_shrink,
                     text(Utils::format_value(state.peek<int8_t>(pos))) |
                         flex_grow}),
               hbox({text(" u8: ") | color(Color::Cyan) | flex_shrink,
                     text(Utils::format_value(state.peek<uint8_t>(pos))) |
                         flex_grow}),
               hbox({text("i16: ") | color(Color::Yellow) | flex_shrink,
                     text(Utils::format_value(state.peek<int16_t>(pos))) |
                         flex_grow}),
               hbox({text("u16: ") | color(Color::Magenta) | flex_shrink,
                     text(Utils::format_value(state.peek<uint16_t>(pos))) |
                         flex_grow}),
               hbox({text("i32: ") | color(Color::Red) | flex_shrink,
                     text(Utils::format_value(state.peek<int32_t>(pos))) |
                         flex_grow}),
               hbox({text("u32: ") | color(Color::Blue) | flex_shrink,
                     text(Utils::format_value(state.peek<uint32_t>(pos))) |
                         flex_grow}),
               hbox({text("i64: ") | color(Color::Red) | flex_shrink,
                     text(Utils::format_value(state.peek<int64_t>(pos))) |
                         flex_grow}),
               hbox({text("u64: ") | color(Color::Blue) | flex_shrink,
                     text(Utils::format_value(state.peek<uint64_t>(pos))) |
                         flex_grow}),
               hbox({text("f32: ") | color(Color::White) | flex_shrink,
                     text(Utils::format_value(state.peek<float>(pos))) |
                         flex_grow}),
               hbox({text("f64: ") | color(Color::White) | flex_shrink,
                     text(Utils::format_value(state.peek<double>(pos))) |
                         flex_grow}),
           }) |
           border | size(WIDTH, EQUAL, 30); // 固定右侧宽度
  });
}

inline Component DataReadHistoryBar(AppState &state) {
  return Renderer([&] {
    size_t pos = state.cursor_pos;
    std::string addr_str = fmt::format("{:08X}", pos);
    std::vector<Record> records = state.get_read_history();
    // 生成历史记录元素
    Elements history_lines;
    for (const auto &record : records) {
      history_lines.push_back(
          hbox({// 地址部分
                text(fmt::format("{:08X}:", record.index)) |
                    color(Color::Green) | flex_shrink,
                // 类型标签
                text(" " + record.type_name + ": ") | color(Color::Green),
                // 数据值
                text(Utils::format_any(record.data)) | flex_grow}));
    }
    return vbox(std::move(history_lines)) | border |
           size(WIDTH, EQUAL, 30); // 固定右侧宽度
  });
}

inline Component CommandLine(AppState &_state, std::string &command_input) {
  (void)_state;

  // 初始化所有InputOption字段
  ftxui::InputOption options;
  options.placeholder = "Command (set be/le/pos <num>)";
  options.password = false;
  options.multiline = false; // 明确关闭多行模式
  options.transform = [](ftxui::InputState state) {
    // 当显示占位符时应用灰色
    if (state.is_placeholder) {
      return state.element | color(ftxui::Color::GrayDark);
    }
    // 正常输入状态保持默认颜色
    return state.element | color(ftxui::Color::White);
  };
  options.on_enter = [] {}; // 空回车处理函数

  auto input = Input(&command_input, options);

  return Renderer(input, [=] {
    return hbox({text(">|") | color(Color::Green), input->Render() | flex}) |
           border | size(HEIGHT, EQUAL, 3);
  });
}
} // namespace UIComponents