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

template <typename T>
T BinaryEditor::peek()
{
    if (m_read_index + sizeof(T)> m_file_size)
    {
        throw std::out_of_range("Read operation exceeds file size");
    }

    // 执行读取
    file.seekg(m_read_index);
    T data;
    file.read(reinterpret_cast<char *>(&data), sizeof(T));
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

template std::vector<uint8_t> BinaryEditor::read<uint8_t>(size_t);
template std::vector<uint16_t> BinaryEditor::read<uint16_t>(size_t);
template std::vector<uint32_t> BinaryEditor::read<uint32_t>(size_t);
template std::vector<uint64_t> BinaryEditor::read<uint64_t>(size_t);

template std::vector<int8_t> BinaryEditor::read<int8_t>(size_t);
template std::vector<int16_t> BinaryEditor::read<int16_t>(size_t);
template std::vector<int32_t> BinaryEditor::read<int32_t>(size_t);
template std::vector<int64_t> BinaryEditor::read<int64_t>(size_t);

template std::vector<float> BinaryEditor::read<float>(size_t);
template std::vector<double> BinaryEditor::read<double>(size_t);



template uint8_t BinaryEditor::peek<uint8_t>();
template uint16_t BinaryEditor::peek<uint16_t>();
template uint32_t BinaryEditor::peek<uint32_t>();
template uint64_t BinaryEditor::peek<uint64_t>();

template int8_t BinaryEditor::peek<int8_t>();
template int16_t BinaryEditor::peek<int16_t>();
template int32_t BinaryEditor::peek<int32_t>();
template int64_t BinaryEditor::peek<int64_t>();

template float BinaryEditor::peek<float>();
template double BinaryEditor::peek<double>();