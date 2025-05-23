#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>
#include <vector>
#include <string>

#include "AppState.hpp"

using namespace ftxui;
// ========== 事件处理 ==========
namespace EventHandlers
{
    auto HandleNavigation(AppState &state, ftxui::ScreenInteractive &screen)
    {
        return [&](const Event &event)
        {
            if (event == Event::PageUp)
            {
                state.current_page = std::max(0, int(state.current_page) - 1);
                return true;
            }
            if (event == Event::PageDown)
            {
                state.current_page = std::min(state.total_pages(Terminal::Size().dimy),
                                              state.current_page + 1);
                return true;
            }
            if (event == Event::ArrowLeft && state.cursor_pos > 0)
            {
                --state.cursor_pos;
                return true;
            }
            if (event == Event::ArrowRight && state.cursor_pos < state.data.size() - 1)
            {
                ++state.cursor_pos;
                return true;
            }
            if (event == Event::ArrowUp && state.cursor_pos >= state.bytes_per_line)
            {
                state.cursor_pos -= state.bytes_per_line;
                return true;
            }
            if (event == Event::ArrowDown && state.cursor_pos + state.bytes_per_line < state.data.size() - 1)
            {
                state.cursor_pos += state.bytes_per_line;
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
}