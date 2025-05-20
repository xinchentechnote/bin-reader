#include "EditorView.hpp"
#include <ftxui/component/component.hpp>

using namespace ftxui;

Component EditorView(BinaryEditor& model) {
    return Renderer([&] {
        return vbox({
            text("Binary File Editor") | bold | center,
            separator(),
            hbox({
                text("Offset: " + std::to_string(model.offset())),
                text(" | File Size: " + std::to_string(model.size()))
            }),
            separator(),
            // 数据展示区域
            vbox({
                text("Data Preview:"),
                // 动态数据渲染...
            }) | flex,
            separator(),
            // 命令输入框
            input("Enter command (e.g. r i32 4):", "")
        });
    });
}