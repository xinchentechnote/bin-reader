#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>
#include <vector>
#include <string>

using namespace ftxui;

// ========== 数据模型 ==========
struct AppState
{
    std::vector<uint8_t> data;  // 文件数据
    size_t cursor_pos = 0;      // 当前光标位置
    size_t bytes_per_line = 16; // 每行显示字节数
    size_t current_page = 0;    // 当前页
    std::string status_msg;     // 状态消息

    // 计算总页数
    [[nodiscard]] size_t total_pages(const int term_height) const
    {
        const size_t lines = (data.size() + bytes_per_line - 1) / bytes_per_line;
        return lines / std::max(1, term_height - 4);
    }
};

// ========== UI组件 ==========
namespace UIComponents
{
    Component HexView(AppState &state)
    {
        return Renderer([&]
                        {
            const auto term = Terminal::Size();
            const int term_height = term.dimy;
            const size_t content_height = static_cast<size_t>(std::max(1, term_height - 4));
            const size_t start_line = state.current_page * content_height;

            Elements lines;
            for (size_t i = 0; i < content_height; ++i) {
                const size_t addr = (start_line + i) * state.bytes_per_line;
                if (addr >= state.data.size()) break;

                Elements hex, ascii;
                for (size_t j = 0; j < state.bytes_per_line; ++j) {
                    const size_t pos = addr + j;
                    const bool is_active = (pos == state.cursor_pos);
                    
                    if (pos >= state.data.size()) {
                        hex.push_back(text("   "));
                        ascii.push_back(text(" "));
                        continue;
                    }

                    const uint8_t byte = state.data[pos];
                    hex.push_back(text(fmt::format("{:02x} ", byte)) | (is_active ? inverted : nothing));
                    ascii.push_back(text(std::string(1, (byte >= 0x20 && byte <= 0x7E) ? char(byte) : '.')) 
                        | color(Color::Yellow) | (is_active ? inverted : nothing));
                }

                lines.push_back(hbox({
                    text(fmt::format("{:08x}: ", addr)) | color(Color::Blue),
                    hbox(hex),
                    text("│") | color(Color::GrayDark),
                    hbox(ascii)
                }));
            }

            return vbox(lines); });
    }

    Component StatusBar(AppState &state)
    {
        return Renderer([&]
                        { return hbox({
                              text(fmt::format(" Pos: 0x{:08x} ", state.cursor_pos)) | bgcolor(Color::DarkBlue),
                              text(fmt::format(" Page: {}/{} ", state.current_page + 1,
                                               state.total_pages(Terminal::Size().dimy) + 1)) |
                                  bgcolor(Color::DarkGreen),
                              text(fmt::format(" {} ", state.status_msg)) | bgcolor(Color::DarkRed) | flex,
                          }); });
    }
}

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