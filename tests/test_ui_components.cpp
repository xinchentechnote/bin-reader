#include <gtest/gtest.h>
#include <ftxui/dom/node.hpp>
#include "AppState.hpp"
#include "UIComponents.hpp"

using namespace ftxui;

class UIComponentsTest : public ::testing::Test {
protected:
    AppState state;
    
    std::string RenderHexView() {
        auto component = UIComponents::HexView(state);
        Element element = component->Render();
        auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(element));
        Render(screen, element);
        return screen.ToString();
    }
};

TEST_F(UIComponentsTest, HexViewRendersAddressAndData) {
    state.data = {0x00, 0x11, 0x22, 0x33};
    state.bytes_per_line = 4;
    
    const std::string output = RenderHexView();
    
    // 验证地址
    EXPECT_NE(output.find("00000000:"), std::string::npos) 
        << "Actual output:\n" << output;

    std::cout << output << std::endl;
    
}