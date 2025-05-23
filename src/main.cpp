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

    // 初始化测试数据
    state.data.resize(512);
    for (size_t i = 0; i < state.data.size(); ++i)
    {
        state.data[i] = i % 256;
    }

    // 构建UI
    auto layout = Container::Vertical({
        UIComponents::StatusBar(state),
        UIComponents::HexView(state),
    });

    // 事件处理
    auto component = CatchEvent(layout, EventHandlers::HandleNavigation(state, screen));

    screen.Loop(component);
    return 0;
}