#pragma once
#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "Utils.hpp"

using namespace ftxui;
// ========== 事件处理 ==========
namespace EventHandlers {
auto HandleNavigation(AppState &state, ftxui::ScreenInteractive &screen) {
  return [&](const Event &event) {
    if (event == ftxui::Event::PageUp)
      return state.pre_page();
    if (event == ftxui::Event::PageDown)
      return state.next_page();
    if (event == ftxui::Event::ArrowLeft)
      return state.pre();
    if (event == ftxui::Event::ArrowRight)
      return state.next();
    if (event == ftxui::Event::ArrowUp)
      return state.pre_line();
    if (event == ftxui::Event::ArrowDown)
      return state.next_line();
    if (event == ftxui::Event::Escape) {
      screen.Exit();
      return true;
    }
    return false;
  };
}

auto HandleCommands(AppState &state, std::string &command_input,
                    ftxui::ScreenInteractive &screen) {
  return [&](const Event &event) {
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
          return ReaderFactory::instance().read(state, type);
        } catch (const std::out_of_range &e) {
          state.status_msg = "Read failed: Out of range";
        }
      }

      if (cmd == "q" || cmd == "quit") {
        screen.Exit();
      }

      command_input.clear();
      return true;
    }

    return false;
  };
}
} // namespace EventHandlers