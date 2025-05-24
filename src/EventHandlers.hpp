#pragma once

#include <functional>                   // for std::function
#include <string>                       // for std::string
#include <ftxui/component/screen_interactive.hpp>
#include "AppState.hpp"

namespace EventHandlers {
  using EventHandlerFn = std::function<bool(const ftxui::Event&)>;

  EventHandlerFn HandleCommands(AppState& state,
                                std::string& cmd,
                                ftxui::ScreenInteractive& screen);

  EventHandlerFn HandleNavigation(AppState& state,
                                  ftxui::ScreenInteractive& screen);
}  // namespace EventHandlers
