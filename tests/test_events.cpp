#include <gtest/gtest.h>
#include <ftxui/screen/screen.hpp>
#include "AppState.hpp"
#include "EventHandlers.hpp"

TEST(EventHandlersTest, PageNavigation) {
    AppState state;
    state.data.resize(1024);
    auto screen = ftxui::ScreenInteractive::TerminalOutput();
    
    auto handler = EventHandlers::HandleNavigation(state, screen);
    handler(ftxui::Event::PageDown);
    ASSERT_EQ(state.current_page, 1);
    
    handler(ftxui::Event::PageUp);
    ASSERT_EQ(state.current_page, 0);
}