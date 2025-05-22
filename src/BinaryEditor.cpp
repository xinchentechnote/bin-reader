#include "BinaryEditor.hpp"
#include <fstream>
#include <stdexcept>

BinaryEditor::BinaryEditor(const std::string &filename)
    : filename(filename), m_read_index(0), m_file_size(0)
{
    file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    load_file_size();
}

BinaryEditor::~BinaryEditor()
{
    if (file.is_open())
    {
        file.close();
    }
}

void BinaryEditor::load_file_size()
{
    file.seekg(0, std::ios::end);
    m_file_size = file.tellg();
    file.seekg(m_read_index);
}

//------------------ 读取操作 ------------------
template <typename T>
std::vector<T> BinaryEditor::read(size_t count)
{
    if (m_read_index + sizeof(T) * count > m_file_size)
    {
        throw std::out_of_range("Read operation exceeds file size");
    }

    // 记录读取历史
    read_history.push({static_cast<uint32_t>(m_read_index), sizeof(T) * count});
    // 执行读取
    file.seekg(m_read_index);
    std::vector<T> data(count);
    file.read(reinterpret_cast<char *>(data.data()), sizeof(T) * count);
    m_read_index += sizeof(T) * count;
    return data;
}

//------------------ 撤销操作 ------------------
void BinaryEditor::undo()
{
    undor();
}

void BinaryEditor::undor()
{
    if (read_history.empty())
        return;

    // 撤销读取：回退读取索引
    Record record = read_history.top();
    m_read_index = record.index;
    read_history.pop();
}

//------------------ 索引管理 ------------------
size_t BinaryEditor::read_index() const
{
    return m_read_index;
}

void BinaryEditor::set_read_index(uint32_t index)
{
    if (index > m_file_size)
        throw std::out_of_range("Invalid read index");
    m_read_index = index;
    file.seekg(m_read_index);
}

size_t BinaryEditor::size() const
{
    return m_file_size;
}

//------------------ 显式模板实例化 ------------------
template std::vector<uint32_t> BinaryEditor::read<uint32_t>(size_t);
template std::vector<int32_t> BinaryEditor::read<int32_t>(size_t);
template std::vector<uint8_t> BinaryEditor::read<uint8_t>(size_t);