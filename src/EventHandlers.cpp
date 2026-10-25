#include "EventHandlers.hpp"
#include "Command.hpp"
#include "Utils.hpp"
#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream> // for std::istringstream

using namespace ftxui;

namespace EventHandlers {

EventHandlerFn HandleNavigation(AppState &state,
                                ftxui::ScreenInteractive &screen) {
  return [&](const Event &event) {
    if (event.character() == ".") {
      state.apply_command(state.last_command);
      return true;
    }

    if (event == ftxui::Event::Escape) {
      screen.Exit();
      return true;
    }

    Command cmd;
    if (event == ftxui::Event::PageUp)
      cmd.type = CommandType::PageUp;
    else if (event == ftxui::Event::PageDown)
      cmd.type = CommandType::PageDown;
    else if (event == ftxui::Event::ArrowLeft)
      cmd.type = CommandType::MoveLeft;
    else if (event == ftxui::Event::ArrowRight)
      cmd.type = CommandType::MoveRight;
    else if (event == ftxui::Event::ArrowUp)
      cmd.type = CommandType::MoveUp;
    else if (event == ftxui::Event::ArrowDown)
      cmd.type = CommandType::MoveDown;
    else
      return false; // 不属于导航按键：交给命令行 Input 处理

    state.last_command = cmd;
    state.apply_command(cmd);
    return true;
  };
}

EventHandlerFn HandleCommands(AppState &state, std::string &command_input,
                              ftxui::ScreenInteractive &screen) {
  return [&](const Event &event) {
    if (event == Event::Return) {
      ParsedCommand cmd = ParsedCommand::parse(command_input);
      if (!cmd.name.empty()) {
        bool handled = CommandRegistry::instance().dispatch(cmd, state);
        if (!handled)
          state.status_msg = "Unknown command: " + cmd.name;
      }

      if (state.exit_requested)
        screen.Exit();

      command_input.clear();
      return true;
    }

    return false;
  };
}

} // namespace EventHandlers
