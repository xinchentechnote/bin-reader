#pragma once

#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "EventHandlers.hpp"
#include "Utils.hpp"

using namespace ftxui;

namespace UIComponents {
  // → You forgot to declare MainUi here:
  Component MainUi(AppState& state, std::string& cmd, ScreenInteractive& screen);

  Component StatusBar(AppState& state);
  Component HexView(AppState& state);
  Component DataPreviewBar(AppState& state);
  Component DataReadHistoryBar(AppState& state);
  Component CommandLine(AppState& state, std::string& command_input);
}  // namespace UIComponents
