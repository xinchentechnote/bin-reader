#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include "CLI11.hpp"
#include <string>
#include <vector>

#include "AppState.hpp"
#include "EventHandlers.hpp"
#include "UIComponents.hpp"

using namespace ftxui; // 统一使用命名空间

int main(int argc,char** argv) {
   CLI::App app{"bin-reader"};
    std::string file_path;
    
    // 添加命令行选项
    app.add_option("-f,--file", file_path, "Binary file to load")
        ->required()
        ->check(CLI::ExistingFile);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e); // 自动处理帮助信息和错误
    }

  auto screen = ScreenInteractive::Fullscreen();
  AppState state;
  std::string cmd;
  state.load_file(file_path); // 加载文件
  // 构建UI组件（修正参数传递）
  auto status_bar = UIComponents::StatusBar(state);
  auto hex_view = UIComponents::HexView(state);
  auto data_preview = UIComponents::DataPreviewBar(state);
  auto data_history = UIComponents::DataReadHistoryBar(state);
  auto command_line = UIComponents::CommandLine(state, cmd) | size(HEIGHT, EQUAL, 3);
  auto workspace = Container::Horizontal({hex_view | flex | size(WIDTH, EQUAL, 150),
                                  data_preview | size(WIDTH, EQUAL, 35),
                                  data_history | size(WIDTH, EQUAL, 35)}) | flex;
  auto layout =
      Container::Vertical(
          {status_bar,
           workspace,
           command_line}) |
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