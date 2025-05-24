#include "AppState.hpp"
#include "UIComponents.hpp"
#include <ftxui/component/screen_interactive.hpp>
#include <gtest/gtest.h>

using namespace ftxui;

TEST(IntegrationTest, BasicRendering) {
  AppState state;
  state.data = {0x48, 0x65, 0x6C, 0x6C}; // "Hell"
  state.bytes_per_line = 4;

  // 获取组件的渲染元素
  auto component = UIComponents::HexView(state);
  Element element = component->Render();

  // 创建屏幕并渲染
  auto screen = Screen::Create(Dimension::Full(),      // 宽度
                               Dimension::Fit(element) // 高度
  );
  Render(screen, element);

  // 验证输出
  std::string output = screen.ToString();
  EXPECT_NE(output.find("00000000:"), std::string::npos) << "Actual output:\n"
                                                         << output;
  std::cout << output << std::endl;
}