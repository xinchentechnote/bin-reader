#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>
#include <vector>
#include <stack>
#include <string>
#include <any>
#include <iomanip>
#include <sstream>

using namespace ftxui;
// ========== 数据模型 ==========

struct Record
{
    size_t index;          // 读取起始位置
    std::any data;         // 存储任何类型的数据
    std::string type_name = std::string(""); // 类型名称

    // 模板构造函数
    template <typename T>
    Record(size_t idx, T value)
        : index(idx),
          data(value) {}
    
    // 格式化描述信息
    std::string description() const {
        std::stringstream ss;
        
        // 格式地址为8位十六进制
        ss << std::hex << std::setw(8) << std::setfill('0') 
           << index << ":" << type_name << ":";
        
        // // 类型安全的数据转换
        // if (type_name == TypeName<uint8_t>::value) {
        //     ss << static_cast<int>(std::any_cast<uint8_t>(data));
        // } else if (type_name == TypeName<int16_t>::value) {
        //     ss << std::any_cast<int16_t>(data);
        // } else if (type_name == TypeName<float>::value) {
        //     ss << std::fixed << std::setprecision(2) 
        //        << std::any_cast<float>(data);
        // } // 其他类型类似处理...
        
        return ss.str();
    }
};

struct AppState
{
    std::vector<uint8_t> data;    // 文件数据
    size_t cursor_pos = 0;        // 当前光标位置
    size_t bytes_per_line = 16;   // 每行显示字节数
    size_t current_page = 0;      // 当前页
    std::string status_msg;       // 状态消息
    bool is_little_endian = true; // 默认小端序
    std::stack<Record> read_history;

    // 计算总页数
    [[nodiscard]] size_t total_pages(const int term_height) const
    {
        const size_t lines = (data.size() + bytes_per_line - 1) / bytes_per_line;
        return lines / std::max(1, term_height - 4);
    }

    // 通用数据读取函数
    template <typename T>
    T peek(size_t pos) const
    {
        // if (pos + sizeof(T) > data.size())
        //     return "N/A";

        T value;
        memcpy(&value, &data[pos], sizeof(T));

        // 需要处理字节序转换
        if (!is_little_endian)
        {
            reverse_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(T));
        }

        return value;
    }

    // 通用数据读取函数
    template <typename T>
    T read(size_t pos)
    {
        // if (pos + sizeof(T) > data.size())
        //     return "N/A";

        T value = peek<T>(pos);
        // 添加记录到历史堆栈
        read_history.push(Record{pos, value});
        cursor_pos  += sizeof(T);
        return value;
    }

    std::vector<Record> get_read_history() const {
        std::vector<Record> history;
        auto temp = read_history; // 复制栈
        
        // 将栈转换为列表（先进先出顺序）
        while (!temp.empty()) {
            history.insert(history.begin(), temp.top()); // 插入到头部
            temp.pop();
        }
        
        return history;
    }


private:
    // 字节序反转
    void reverse_bytes(uint8_t *bytes, size_t size) const
    {
        for (size_t i = 0; i < size / 2; ++i)
        {
            std::swap(bytes[i], bytes[size - 1 - i]);
        }
    }
   
};