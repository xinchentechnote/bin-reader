#pragma once
#include <fstream>
#include <vector>
#include <stack>
#include <cstdint>
#include <stdexcept>

class BinaryEditor {
public:
    BinaryEditor(const std::string& filename);
    ~BinaryEditor();

    // 基础读写操作
    template<typename T>
    std::vector<T> read(size_t count = 1);
    template<typename T>
    void write(const std::vector<T>& values);
    
    // 撤销操作
    void undo();
    
    // 偏移量管理
    size_t offset() const;
    void set_offset(size_t new_offset);
    size_t size() const;

private:
    std::fstream file;
    std::string filename;
    size_t current_offset;
    size_t file_size;
    std::stack<std::vector<std::pair<size_t, std::vector<char>>>> write_history;
    void load_file_size();
};