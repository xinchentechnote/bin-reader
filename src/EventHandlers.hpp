#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>
#include <vector>
#include <string>

#include "AppState.hpp"
#include "Utils.hpp"

using namespace ftxui;
// ========== 事件处理 ==========
namespace EventHandlers
{
    auto HandleNavigation(AppState &state, ftxui::ScreenInteractive &screen)
    {
        return [&](const Event &event)
        {
            const int term_height = Terminal::Size().dimy;
            const size_t content_height = static_cast<size_t>(std::max(1, term_height - 4));
            if (event == Event::PageUp)
            {
                size_t tmp = state.current_page;
                state.current_page = std::max(0, int(state.current_page) - 1);
                if (tmp != state.current_page)
                {
                    state.cursor_pos -= term_height * state.bytes_per_line;
                }
                return true;
            }
            if (event == Event::PageDown)
            {
                size_t tmp = state.current_page;
                state.current_page = std::min(state.total_pages(term_height),
                                              state.current_page + 1);
                if (tmp != state.current_page)
                {
                    state.cursor_pos += term_height * state.bytes_per_line;
                }
                return true;
            }
            if (event == Event::ArrowLeft && state.cursor_pos > 0)
            {
                --state.cursor_pos;
                state.current_page = state.cursor_pos / state.bytes_per_line / content_height;
                return true;
            }
            if (event == Event::ArrowRight && state.cursor_pos < state.data.size() - 1)
            {
                ++state.cursor_pos;
                state.current_page = state.cursor_pos / state.bytes_per_line / content_height;
                return true;
            }
            if (event == Event::ArrowUp && state.cursor_pos >= state.bytes_per_line)
            {
                state.cursor_pos -= state.bytes_per_line;
                state.current_page = state.cursor_pos / state.bytes_per_line / content_height;
                return true;
            }
            if (event == Event::ArrowDown && state.cursor_pos + state.bytes_per_line < state.data.size() - 1)
            {
                state.cursor_pos += state.bytes_per_line;
                state.current_page = state.cursor_pos / state.bytes_per_line / content_height;
                return true;
            }
            if (event == Event::Character('q') ||
                event == Event::Escape)
            {
                screen.Exit();
                return true;
            }
            return false;
        };
    }

    auto HandleCommands(AppState& state, std::string& command_input, ftxui::ScreenInteractive &screen) {
    return [&](const Event& event) {
        (void) screen;
        if (event == Event::Return) {
            // 解析命令
            std::istringstream iss(command_input);
            std::string cmd;
            iss >> cmd;

            if (cmd == "set") {
                std::string subcmd;
                iss >> subcmd;
                
                if (subcmd == "be") {
                    state.is_little_endian = false;
                    state.status_msg = "Big-endian mode";
                } else if (subcmd == "le") {
                    state.is_little_endian = true;
                    state.status_msg = "Little-endian mode";
                } else if (subcmd == "pos") {
                    size_t new_pos;
                    if (iss >> new_pos) {
                        if (new_pos < state.data.size()) {
                            state.cursor_pos = new_pos;
                            state.status_msg = fmt::format("Position set to 0x{:X}", new_pos);
                        } else {
                            state.status_msg = "Invalid position!";
                        }
                    }
                }
            }

            if (cmd == "r") {
                std::string type;
                iss >> type;
                
                try {
                    if (type == "u32") {
                        auto value = state.read<uint32_t>(state.cursor_pos);
                        state.status_msg = fmt::format("Read u32: {} @ 0x{:X}", 
                            Utils::format_value(value), state.cursor_pos);
                        return true;
                    }
                } catch (const std::out_of_range& e) {
                    state.status_msg = "Read failed: Out of range";
                }
            }
            
            command_input.clear();
            return true;
        }
        return false;
    };
}
}