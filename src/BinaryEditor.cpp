#include "BinaryEditor.hpp"
#include <fstream>
#include <stdexcept>

template <typename T> struct TypeName { static const char* value; };

// 特化基础类型
template<> const char* TypeName<uint8_t>::value = "uint8_t";
template<> const char* TypeName<uint16_t>::value = "uint16_t";
template<> const char* TypeName<uint32_t>::value = "uint32_t";
template<> const char* TypeName<uint64_t>::value = "uint64_t";
template<> const char* TypeName<int8_t>::value = "int8_t";
template<> const char* TypeName<int16_t>::value = "int16_t";
template<> const char* TypeName<int32_t>::value = "int32_t";
template<> const char* TypeName<int64_t>::value = "int64_t";
template<> const char* TypeName<float>::value = "float";
template<> const char* TypeName<double>::value = "double";
template<> const char* TypeName<bool>::value = "bool";
template<> const char* TypeName<std::string>::value = "string";

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

    // 执行读取
    file.seekg(m_read_index);
    std::vector<T> data(count);
    file.read(reinterpret_cast<char *>(data.data()), sizeof(T) * count);

    Record record(m_read_index);
    record.type_name = TypeName<T>::value;
    if (count > 1) record.type_name += "[" + std::to_string(count) + "]";
    record.data = data;
    record.size_bytes = sizeof(T) * count;
    read_history.push(record);
    m_read_index += sizeof(T) * count;

    return data;
}

template <typename T>
T BinaryEditor::peek()
{
    if (m_read_index + sizeof(T) > m_file_size)
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

std::string BinaryEditor::read_fixed_string(size_t n)
{
    if (m_read_index + n > m_file_size)
    {
        throw std::out_of_range("Read operation exceeds file size");
    }

    Record record(m_read_index);
    // 读取原始字节
    file.seekg(m_read_index);
    std::vector<char> buffer(n);
    file.read(buffer.data(), n);
    m_read_index += n;
    
    // 转换为字符串（UTF-8）
    std::string data = std::string(buffer.data(), n);
    // 记录读取历史
    record.type_name = TypeName<std::string>::value;
    record.type_name += "[" + std::to_string(n) + "]";
    record.data = data;
   
    record.size_bytes = n;
    read_history.push(record);
    return data;
}

std::string BinaryEditor::peek_fixed_string(size_t n)
{
    if (m_read_index + n > m_file_size)
    {
        throw std::out_of_range("Peek operation exceeds file size");
    }

    auto original_pos = file.tellg();
    file.seekg(m_read_index);

    std::vector<char> buffer(n);
    file.read(buffer.data(), n);

    file.seekg(original_pos);
    return std::string(buffer.data(), n);
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

template <typename LengthType>
std::string BinaryEditor::read_length_prefixed_string() {
    // 记录整体操作开始位置
    const size_t start_index = m_read_index;

    try {
        // 读取长度前缀
        LengthType len = read<LengthType>()[0];
        
        // 读取字符串内容
        std::string str = read_fixed_string(len);

        // 用整体操作替换两个独立记录
        read_history.pop();  // 移除固定字符串记录
        read_history.pop();  // 移除长度前缀记录

        Record record(start_index);
        record.type_name = TypeName<LengthType>::value;
        record.type_name += "@string";
        record.data = str;
        record.size_bytes = sizeof(LengthType) + len;
        read_history.push(record);

        return str;
    } catch (...) {
        // 发生异常时恢复读取索引
        m_read_index = start_index;
        throw;
    }
}

// 显式实例化常用长度类型
template std::string BinaryEditor::read_length_prefixed_string<uint8_t>();
template std::string BinaryEditor::read_length_prefixed_string<uint16_t>();
template std::string BinaryEditor::read_length_prefixed_string<uint32_t>();