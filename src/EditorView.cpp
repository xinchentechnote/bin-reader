#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <BinaryEditor.hpp>
using namespace ftxui;

Component EditorView([[maybe_unused]] BinaryEditor& editor) {
  // 创建输入组件
  auto input = Input("Enter command (e.g. r i32 4):", "");

  return Renderer(input, [=] {
    return vbox({
      text("Binary File Editor") | bold | center,
      separator(),
      input->Render(),  // 正确渲染组件
      separator(),
      text("Output Area:")
    });
  });
}