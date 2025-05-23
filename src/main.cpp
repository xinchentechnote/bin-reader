#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>
#include <vector>
#include <string>

#include "AppState.hpp"
#include "EventHandlers.hpp"
#include "UIComponents.hpp"

// ========== 主函数 ==========
int main()
{
    auto screen = ScreenInteractive::Fullscreen();
    AppState state;
    std::string cmd;
    // 初始化测试数据
    state.data.resize(512);
    for (size_t i = 0; i < state.data.size(); ++i)
    {
        state.data[i] = i % 256;
    }
    auto commandLine = UIComponents::CommandLine(cmd);
    // 构建UI
    auto layout = Container::Vertical({
        UIComponents::StatusBar(state),
        Container::Horizontal({
            UIComponents::HexView(state),
            UIComponents::DataPreviewBar(state)
        }),
       commandLine
    });

    // 事件处理
    auto component = ftxui::CatchEvent(layout, [&](const ftxui::Event& event) {
        // 先处理命令输入
        if (EventHandlers::HandleCommands(state, cmd)(event)) {
            return true;
        }
        // 再处理导航事件
        return EventHandlers::HandleNavigation(state, screen)(event);
    });

    screen.Loop(component);
    return 0;
}