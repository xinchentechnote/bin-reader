#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>
#include <vector>
#include <string>

#include "AppState.hpp"

using namespace ftxui;
// ========== UI组件 ==========
namespace UIComponents
{
    inline Component HexView(AppState &state)
    {
        return Renderer([&]
                        {
            const auto term = Terminal::Size();
            const int term_height = term.dimy;
            const size_t content_height = static_cast<size_t>(std::max(1, term_height - 4));
            const size_t start_line = state.current_page * content_height;

            Elements lines;
            for (size_t i = 0; i < content_height; ++i) {
                const size_t addr = (start_line + i) * state.bytes_per_line;
                if (addr >= state.data.size()) break;

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
                    hex.push_back(text(fmt::format("{:02X} ", byte)) | (is_active ? inverted : nothing));
                    ascii.push_back(text(std::string(1, (byte >= 0x20 && byte <= 0x7E) ? char(byte) : '.')) 
                        | color(Color::Yellow) | (is_active ? inverted : nothing));
                }

                lines.push_back(hbox({
                    text(fmt::format("{:08x}: ", addr)) | color(Color::Blue),
                    hbox(hex),
                    text("│") | color(Color::GrayDark),
                    hbox(ascii)
                }));
            }

            return vbox(lines); }) | border;
    }

    inline Component StatusBar(AppState &state)
    {
        return Renderer([&]
                        { return hbox({
                              text(fmt::format(" Pos: 0x{:08x} ", state.cursor_pos)) | bgcolor(Color::DarkBlue),
                              text(fmt::format(" Page: {}/{} ", state.current_page + 1,
                                               state.total_pages(Terminal::Size().dimy) + 1)) |
                                  bgcolor(Color::DarkGreen),
                              text(fmt::format(" {} ", state.status_msg)) | bgcolor(Color::DarkRed) | flex,
                          }); });
    }

    inline Component DataPreviewBar(AppState& state) {
        return Renderer([&] {
            size_t pos = state.cursor_pos;
            auto endian_str = state.is_little_endian ? "LE" : "BE";
            return vbox({
                       hbox({text("Endian:") | bold | color(Color::GrayDark),
                             text(endian_str)}),
                       hbox({text(" i8: ") | color(Color::Green) | flex_shrink,
                             text(state.read_value<int8_t>(pos)) | flex_grow}),
                       hbox({text(" u8: ") | color(Color::Cyan) | flex_shrink,
                             text(state.read_value<uint8_t>(pos)) | flex_grow}),
                       hbox({text("i16: ") | color(Color::Yellow) | flex_shrink,
                             text(state.read_value<int16_t>(pos)) | flex_grow}),
                       hbox({text("u16: ") | color(Color::Magenta) | flex_shrink,
                             text(state.read_value<uint16_t>(pos)) | flex_grow}),
                       hbox({text("i32: ") | color(Color::Red) | flex_shrink,
                             text(state.read_value<int32_t>(pos)) | flex_grow}),
                       hbox({text("u32: ") | color(Color::Blue) | flex_shrink,
                             text(state.read_value<uint32_t>(pos)) | flex_grow}),
                       hbox({text("i64: ") | color(Color::Red) | flex_shrink,
                             text(state.read_value<int64_t>(pos)) | flex_grow}),
                       hbox({text("u64: ") | color(Color::Blue) | flex_shrink,
                             text(state.read_value<uint64_t>(pos)) | flex_grow}),
                       hbox({text("f32: ") | color(Color::White) | flex_shrink,
                             text(state.read_value<float>(pos)) | flex_grow}),
                       hbox({text("f64: ") | color(Color::White) | flex_shrink,
                             text(state.read_value<double>(pos)) | flex_grow}),
                   }) |
                   border | size(WIDTH, EQUAL, 30); // 固定右侧宽度
        });
    }

    inline Component CommandLine(std::string& command_input) {
        ftxui::InputOption options;
        options.placeholder = "Command (set be/le/pos <num>)";
        options.password = false;
        
        auto input_component = Input(&command_input, options);

        return Renderer(input_component, [&] {
            return hbox({
                text("> ") | color(Color::Green),
                input_component->Render() 
            });
        });
    }
}