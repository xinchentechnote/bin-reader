#pragma once
#include <any>
#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "Utils.hpp"

using namespace ftxui;

// ========== 数据模型 ==========
struct Record {
  size_t index;                            // 读取起始位置
  std::any data;                           // 存储任何类型的数据
  std::string type_name = std::string(""); // 类型名称

  // 模板构造函数
  template <typename T> Record(size_t idx, T value) : index(idx), data(value) {}

  // 格式化描述信息
  std::string description() const {
    std::stringstream ss;

    // 格式地址为8位十六进制
    ss << std::hex << std::setw(8) << std::setfill('0') << index << ":"
       << type_name << ":";

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

struct AppState {
  std::vector<uint8_t> data;    // 文件数据
  size_t cursor_pos = 0;        // 当前光标位置
  size_t bytes_per_line = 16;   // 每行显示字节数
  size_t current_page = 0;      // 当前页
  size_t hex_view_h = 5;        // hex view 高度
  std::string status_msg;       // 状态消息
  bool is_little_endian = true; // 默认小端序
  std::stack<Record> read_history;

  // 计算总页数
  [[nodiscard]] size_t total_pages() const {
    if (data.empty())
      return 0; // Handle empty data case
    // Calculate total lines needed (rounding up)
    const size_t total_lines =
        (data.size() + bytes_per_line - 1) / bytes_per_line;
    // Prevent division by zero and handle cases where hex_view_h is 0
    const size_t lines_per_page = std::max(size_t{1}, hex_view_h);
    // Calculate pages needed (rounding up)
    return (total_lines + lines_per_page - 1) / lines_per_page;
  }

  // 通用数据读取函数
  template <typename T> T peek(size_t pos) const {
    if (pos + sizeof(T) > data.size()) {
      throw std::out_of_range("Attempt to read beyond data bounds");
    }

    T value;
    memcpy(&value, &data[pos], sizeof(T));

    // 需要处理字节序转换
    if (!is_little_endian) {
      reverse_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(T));
    }

    return value;
  }

  // 通用数据读取函数
  template <typename T> T read(size_t pos) {
    T value = peek<T>(pos);
    // 添加记录到历史堆栈
    read_history.push(Record{pos, value});
    move(sizeof(T));
    return value;
  }

  // 读取定长字符串
  std::string read_fixed_string(size_t pos, size_t n) {
    if (pos + n > data.size()) {
      throw std::out_of_range("Read operation exceeds data size");
    }

    // 读取原始字节
    std::string str(data.begin() + pos, data.begin() + pos + n);
    move(n);

    // 记录读取历史
    read_history.push(Record{pos, str});

    return str;
  }

  // 读取长度前缀字符串（模板化长度类型）
  template <typename LengthType>
  std::string read_length_prefixed_string(size_t pos) {
    const size_t start_pos = cursor_pos;

    try {
      // 读取长度前缀
      LengthType len = read<LengthType>(pos);

      // 读取字符串内容
      std::string str = read_fixed_string(pos + sizeof(LengthType), len);

      // 用整体操作替换两个独立记录
      read_history.pop(); // 移除固定字符串记录
      read_history.pop(); // 移除长度前缀记录

      // 创建合并记录
      read_history.push(Record{start_pos, str});

      return str;
    } catch (...) {
      // 发生异常时恢复读取位置
      cursor_pos = start_pos;
      throw;
    }
  }

  //光标移动
  bool set_cursor_pos(size_t new_pos) {
    if (0 < new_pos && new_pos < data.size() - 1) {
      cursor_pos = new_pos;
      update_page();
      return true;
    }
    return false;
  }
  bool move(size_t offset) {
    size_t new_pos = cursor_pos + offset;
    if (data.size() - 1 > new_pos && new_pos > 0) {
      cursor_pos = new_pos;
      update_page();
      return true;
    }
    return false;
  }

  //光标前移
  bool pre() {
    if (cursor_pos > 0) {
      cursor_pos--;
      update_page();
      return true;
    }
    return false;
  }

  //关闭后移
  bool next() {
    if (cursor_pos < data.size() - 1) {
      cursor_pos++;
      update_page();
      return true;
    }
    return false;
  }

  bool pre_line() {
    if (cursor_pos >= bytes_per_line) {
      cursor_pos -= bytes_per_line;
      update_page();
      return true;
    }
    return false;
  }

  bool next_line() {
    if (cursor_pos + bytes_per_line < data.size() - 1) {
      cursor_pos += bytes_per_line;
      update_page();
      return true;
    }
    return false;
  }

  void update_page() {
    current_page = cursor_pos / bytes_per_line / hex_view_h;
  }

  bool next_page() {
    size_t total_pages = this->total_pages();
    if (current_page + 1 < total_pages) {
      current_page++;
      cursor_pos = current_page * bytes_per_line * hex_view_h;
      return true;
    }
    return false;
  }

  bool pre_page() {
    if (current_page > 0) {
      current_page--;
      cursor_pos = current_page * bytes_per_line * hex_view_h;
      return true;
    }
    return false;
  }

  std::vector<Record> get_read_history() const {
    std::vector<Record> history;
    auto temp = read_history;
    while (!temp.empty()) {
      history.insert(history.begin(), temp.top());
      temp.pop();
    }

    return history;
  }

private:
  // 字节序反转
  void reverse_bytes(uint8_t *bytes, size_t size) const {
    for (size_t i = 0; i < size / 2; ++i) {
      std::swap(bytes[i], bytes[size - 1 - i]);
    }
  }
};

class ReaderStrategy {
public:
  virtual ~ReaderStrategy() = default;
  virtual bool read(AppState &state) const = 0;
};

template <typename T> class TypedReader : public ReaderStrategy {
public:
  TypedReader(std::string typeName) : typeName_(std::move(typeName)) {}

  bool read(AppState &state) const override {
    auto orig_pos = state.cursor_pos;
    auto value = state.read<T>(state.cursor_pos);
    state.status_msg = fmt::format("Read {}: {} @ 0x{:X}", typeName_,
                                   Utils::format_value(value), orig_pos);
    return true;
  }

private:
  std::string typeName_;
};

class FixStringReader : public ReaderStrategy {
public:
  bool read(AppState &state) const override {
    try {
      auto orig_pos = state.cursor_pos;
      auto value = state.read_fixed_string(state.cursor_pos, 5);
      state.status_msg = fmt::format("Read char[{}]: {} @ 0x{:X}", 5,
                                     Utils::format_value(value), orig_pos);
      return true;
    } catch (const std::exception &e) {
      state.status_msg = fmt::format("String read failed: {}", e.what());
      return false;
    }
  }
};

template <typename LengthType>
class LengthPrefixedStringReader : public ReaderStrategy {
public:
  bool read(AppState &state) const override {

    auto orig_pos = state.cursor_pos;
    auto value =
        state.read_length_prefixed_string<LengthType>(state.cursor_pos);
    state.status_msg = fmt::format("Read lpstring: {} @ 0x{:X}",
                                   Utils::format_value(value), orig_pos);
    return true;
  }
};

class ReaderFactory {
  using StrategyMap =
      std::unordered_map<std::string, std::unique_ptr<ReaderStrategy>>;
  StrategyMap readers_;

public:
  static const ReaderFactory &instance() {
    static ReaderFactory factory;
    return factory;
  }

  ReaderFactory() {
    emplaceReader<uint8_t>("u8");
    emplaceReader<int8_t>("i8");
    emplaceReader<uint16_t>("u16");
    emplaceReader<int16_t>("i16");
    emplaceReader<uint32_t>("u32");
    emplaceReader<int32_t>("i32");
    emplaceReader<uint64_t>("u64");
    emplaceReader<int64_t>("i64");
    emplaceReader<float>("f32");
    emplaceReader<double>("f64");
    readers_.emplace("string", std::make_unique<FixStringReader>());
    readers_.emplace("string@u8",
                     std::make_unique<LengthPrefixedStringReader<uint8_t>>());
    readers_.emplace("string@u16",
                     std::make_unique<LengthPrefixedStringReader<uint16_t>>());
    readers_.emplace("string@u32",
                     std::make_unique<LengthPrefixedStringReader<uint32_t>>());
    readers_.emplace("string@i8",
                     std::make_unique<LengthPrefixedStringReader<int8_t>>());
    readers_.emplace("string@i16",
                     std::make_unique<LengthPrefixedStringReader<int16_t>>());
    readers_.emplace("string@i32",
                     std::make_unique<LengthPrefixedStringReader<int32_t>>());
  }

  template <typename T> void emplaceReader(const std::string &typeName) {
    readers_.emplace(typeName, std::make_unique<TypedReader<T>>(typeName));
  }

  bool read(AppState &state, const std::string &type) const {
    auto it = readers_.find(type);
    if (it != readers_.end()) {
      return it->second->read(state);
    }
    return false;
  }
};