#pragma once
#include "BinaryEditor.hpp"
#include <ftxui/component/component_base.hpp>

class EditorController : public ftxui::ComponentBase {
public:
    EditorController(BinaryEditor& model);
    
    // 命令解析逻辑
    void parse_command(const std::string& command);
    
private:
    BinaryEditor& model;
    std::string input_buffer;
};