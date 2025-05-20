#include "EditorController.hpp"
#include "BinaryEditor.hpp"
#include <ftxui/component/component.hpp>

EditorController::EditorController(BinaryEditor& model) : model(model) {}

void EditorController::parse_command(const std::string& command) {
    std::istringstream iss(command);
    std::string action;
    iss >> action;

    try {
        if (action == "r") {
            std::string type;
            size_t count = 1;
            iss >> type >> count;
            
            if (type == "i32") {
                auto data = model.read<int32_t>(count);
                // 更新视图数据...
            }
            // 处理其他类型...
            
        } else if (action == "w") {
            // 解析写入命令...
        }
    } catch (const std::exception& e) {
        // 处理错误...
    }
}