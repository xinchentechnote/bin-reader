#pragma once
#include <fstream>
#include <vector>
#include <stack>
#include <cstdint>
#include <stdexcept>

struct Record
{
    u_int32_t index;
    size_t data_size;
};

class BinaryEditor
{
public:
    BinaryEditor(const std::string &filename);
    ~BinaryEditor();

    // 基础读写操作
    template <typename T>
    std::vector<T> read(size_t count = 1);
    template <typename T>
    T peek();

    // 固定长度字符串 (n bytes)
    std::string read_fixed_string(size_t n);
    std::string peek_fixed_string(size_t n);

    // 长度前缀字符串 (prefix_type 可以是 uint8_t/uint16_t/uint32_t)
    template <typename LengthType>
    std::string read_length_prefixed_string();

    // 撤销操作
    void undor();
    void undo();

    // 偏移量管理
    size_t read_index() const;
    void set_read_index(u_int32_t read_index);
    size_t size() const;

private:
    std::fstream file;
    std::string filename;
    size_t m_read_index;
    size_t m_file_size;
    // record read history(read_inex, read_data_size)
    std::stack<Record> read_history;
    void load_file_size();
};