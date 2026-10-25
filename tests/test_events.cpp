#include "AppState.hpp"
#include "EventHandlers.hpp"
#include "UIComponents.hpp"
#include <ftxui/screen/screen.hpp>
#include <gtest/gtest.h>

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

// 导航处理器只应消费它认识的按键。此前它对任意事件都返回 true，
// CatchEvent 因此吞掉了所有字符，命令行 Input 永远收不到键入。
TEST(EventHandlersTest, OnlyNavigationKeysAreIntercepted) {
  AppState state;
  state.data.resize(1024);
  auto screen = ftxui::ScreenInteractive::TerminalOutput();
  auto handler = EventHandlers::HandleNavigation(state, screen);

  EXPECT_FALSE(handler(ftxui::Event::Character('r')));
  EXPECT_FALSE(handler(ftxui::Event::Character('u')));
  EXPECT_FALSE(handler(ftxui::Event::Return));
  EXPECT_TRUE(handler(ftxui::Event::ArrowRight));
  EXPECT_TRUE(handler(ftxui::Event::PageDown));
  EXPECT_TRUE(handler(ftxui::Event::Character('.')));
}

// '.' 重复最近一次导航命令
TEST(EventHandlersTest, DotRepeatsLastNavigationCommand) {
  AppState state;
  state.data.resize(1024);
  auto screen = ftxui::ScreenInteractive::TerminalOutput();
  auto handler = EventHandlers::HandleNavigation(state, screen);

  handler(ftxui::Event::PageDown);
  const size_t page_after_one = state.current_page;
  handler(ftxui::Event::Character('.'));
  EXPECT_EQ(state.current_page, page_after_one + 1);
}