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

EventHandlerFn HandleCommands(AppState &state, std::string &command_input,
                              ftxui::ScreenInteractive &screen) {
  return [&](const Event &event) {
    if (event == Event::Return) {
      ParsedCommand cmd = ParsedCommand::parse(command_input);
      bool handled = CommandRegistry::instance().dispatch(cmd, state);

      if (!handled)
        state.status_msg = "Unknown command: " + cmd.name;

      if (state.exit_requested)
        screen.Exit();

      command_input.clear();
      return true;
    }

    return false;
  };
}

} // namespace EventHandlers
