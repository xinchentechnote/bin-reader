#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "EventHandlers.hpp"
#include "UIComponents.hpp"

using namespace ftxui; // 统一使用命名空间

int main() {
  auto screen = ScreenInteractive::Fullscreen();
  AppState state;
  std::string cmd;

  // 初始化测试数据
  state.data.resize(512);
  for (size_t i = 0; i < state.data.size(); ++i) {
    state.data[i] = i % 256;
  }

  // 构建UI组件（修正参数传递）
  auto status_bar = UIComponents::StatusBar(state);
  auto hex_view = UIComponents::HexView(state);
  auto data_preview = UIComponents::DataPreviewBar(state);
  auto data_history = UIComponents::DataReadHistoryBar(state);
  auto command_line = UIComponents::CommandLine(state, cmd); // 添加state参数

  auto layout =
      Container::Vertical(
          {status_bar,
           Container::Horizontal({hex_view | flex,
                                  data_preview | size(WIDTH, EQUAL, 35),
                                  data_history | size(WIDTH, EQUAL, 35)}) |
               flex,
           command_line | size(HEIGHT, EQUAL, 3)}) |
      flex;
  // 事件处理（修正参数传递）
  screen.PostEvent(Event::Custom); // 触发一次空事件
  auto component = CatchEvent(layout, [&](const Event &event) {
    if (event == Event::Custom) {
      command_line->TakeFocus();
      return true;
    }
    // 处理命令输入（添加screen参数）
    if (EventHandlers::HandleCommands(state, cmd, screen)(event)) {
      return true;
    }
    // 处理导航事件
    return EventHandlers::HandleNavigation(state, screen)(event);
  });

  screen.Loop(component);
  return 0;
}