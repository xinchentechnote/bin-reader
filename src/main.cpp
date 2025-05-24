#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "UIComponents.hpp"

using namespace ftxui; // 统一使用命名空间

int main(int argc, char** argv) {
    try {
        // Initialize application
        const std::string file_path = Utils::ParseCommandLine(argc, argv);
        auto screen = ScreenInteractive::Fullscreen();
        AppState state;
        // Load initial file
        state.load_file(file_path);
        // Setup and run UI
        std::string cmd;
        auto ui = UIComponents::MainUi(state, cmd, screen);
        screen.Loop(ui);
        return 0;
    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
}