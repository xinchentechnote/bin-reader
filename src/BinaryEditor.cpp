#include "BinaryEditor.hpp"
#include <fstream>
#include <stdexcept>

BinaryEditor::BinaryEditor(const std::string& filename) : current_offset(0) {
    file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    load_file_size();
}

BinaryEditor::~BinaryEditor() {
    if (file.is_open()) {
        file.close();
    }
}

void BinaryEditor::load_file_size() {
    file.seekg(0, std::ios::end);
    file_size = file.tellg();
    file.seekg(current_offset);
}

template<typename T>
std::vector<T> BinaryEditor::read(size_t count) {
    if (current_offset + sizeof(T) * count > file_size) {
        throw std::out_of_range("Read operation exceeds file size");
    }

    std::vector<T> data(count);
    file.read(reinterpret_cast<char*>(data.data()), sizeof(T) * count);
    current_offset += sizeof(T) * count;
    return data;
}

template<typename T>
void BinaryEditor::write(const std::vector<T>& values) {
    if (values.empty()) return;

    // 备份原始数据
    const size_t write_size = sizeof(T) * values.size();
    std::vector<char> original_data(write_size);
    
    file.seekg(current_offset);
    file.read(original_data.data(), write_size);
    write_history.push({{current_offset, original_data}});

    // 写入新数据
    file.seekp(current_offset);
    file.write(reinterpret_cast<const char*>(values.data()), write_size);
    file.flush();
    
    current_offset += write_size;
    load_file_size();
}

void BinaryEditor::undo() {
    if (write_history.empty()) return;

    auto [pos, data] = write_history.top().back();
    file.seekp(pos);
    file.write(data.data(), data.size());
    file.flush();
    write_history.pop();
    load_file_size();
}

void BinaryEditor::set_offset(size_t new_offset) {
    if (new_offset > file_size) {
        throw std::out_of_range("Offset exceeds file size");
    }
    current_offset = new_offset;
    file.seekg(new_offset);
}

// 显式实例化模板
template std::vector<uint8_t> BinaryEditor::read<uint8_t>(size_t);
template std::vector<int32_t> BinaryEditor::read<int32_t>(size_t);
template std::vector<uint32_t> BinaryEditor::read<uint32_t>(size_t);
template void BinaryEditor::write<uint8_t>(const std::vector<uint8_t>&);
template void BinaryEditor::write<int32_t>(const std::vector<int32_t>&);