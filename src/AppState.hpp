#pragma once

#include <any>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "Utils.hpp"

using namespace ftxui;

// ========== Record ==========
/// 记录一次读取操作：记录读取位置 + 读到的数据 + 类型名称
struct Record {
  size_t index;                          // 读取起始位置
  std::any data;                         // 存储任何类型的数据
  std::string type_name = std::string(); // 类型名称

  template <typename T>
  Record(size_t idx, T value) : index(idx), data(std::move(value)) {}

  /// 格式化成 “<8 位十六进制地址>:<类型名>:<数据>”
  std::string description() const {
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << index << ":"
       << type_name << ":";
    // 用户可以根据 type_name 再 any_cast 到相应类型并格式化输出
    return ss.str();
  }
};

// ========== AppState ==========
/// 保存整个程序的状态，以及各种读/写、光标移动逻辑
struct AppState {
  std::vector<uint8_t> data;  // 原始文件的二进制数据
  size_t cursor_pos = 0;      // 当前光标位置（字节索引）
  size_t bytes_per_line = 16; // 每行显示的字节数
  size_t current_page = 0;    // 当前页号（从 0 开始）
  size_t hex_view_h = 16; // Hex 视图高度：每页最多显示 hex_view_h 行
  std::string status_msg;          // 状态栏文字
  bool is_little_endian = true;    // 默认小端序
  std::stack<Record> read_history; // 保存所有已读取的记录以便 undo

  std::string file_name;       // 当前打开的文件名
  bool exit_requested = false; // 是否请求退出

  /// 从磁盘加载整个文件到 data 中，并初始化 cursor/page
  void load_file(const std::string &path) {
    data = Utils::read_binary_file(path);
    file_name = std::filesystem::path(path).filename().string();
    cursor_pos = 0;
    current_page = 0;
  }

  /// 计算总页数：等于 ceil(total_lines / hex_view_h)
  [[nodiscard]] size_t total_pages() const {
    if (data.empty())
      return 0;
    const size_t total_lines =
        (data.size() + bytes_per_line - 1) / bytes_per_line;
    const size_t lines_per_page = std::max<size_t>(1, hex_view_h);
    return (total_lines + lines_per_page - 1) / lines_per_page;
  }

  // —— 通用读取/移动 接口 —— //

  /// 〈peek〉：在 pos 处“窥视”一个 T 类型的数据，但不移动光标
  template <typename T> T peek(size_t pos) const {
    if (pos + sizeof(T) > data.size()) {
      throw std::out_of_range("Attempt to read beyond data bounds");
    }
    T value;
    std::memcpy(&value, &data[pos], sizeof(T));
    if (!is_little_endian) {
      reverse_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(T));
    }
    return value;
  }

  /// 〈read〉：从 cursor_pos 处读取一个 T 类型的数据，并记录到
  /// history；光标向前移动 sizeof(T) 字节
  template <typename T> T read(size_t pos) {
    T value = peek<T>(pos);
    read_history.push(Record{pos, value});
    move(sizeof(T));
    return value;
  }

  /// 从 pos 处读取固定长度字符串（长度为 n），并推入 history；光标向前移动 n
  std::string read_fixed_string(size_t pos, size_t n) {
    if (pos + n > data.size()) {
      throw std::out_of_range("Read operation exceeds data size");
    }
    std::string str(data.begin() + pos, data.begin() + pos + n);
    move(n);
    read_history.push(Record{pos, str});
    return str;
  }

  /// 从 pos 处读取“长度前缀字符串”：先读一个 LengthType
  /// 长度，然后再读对应字节数的字符串
  template <typename LengthType>
  std::string read_length_prefixed_string(size_t pos) {
    const size_t start_pos = cursor_pos;
    try {
      LengthType len =
          read<LengthType>(pos); // 已经把 cursor_pos += sizeof(LengthType)
      std::string str = read_fixed_string(pos + sizeof(LengthType), len);
      // 因为我们想把“长度前缀”和“字符串”当作一个整体记录，所以先弹出它们，再合并成一个
      read_history.pop(); // 弹出固定字符串那条
      read_history.pop(); // 弹出长度前缀那条
      read_history.push(Record{start_pos, str});
      return str;
    } catch (...) {
      cursor_pos = start_pos; // 恢复光标
      throw;
    }
  }

  /// 撤销最近一次读取：弹出 read_history，并把 cursor_pos 恢复到该记录的 index
  void undo() {
    if (!read_history.empty()) {
      Record record = read_history.top();
      set_cursor_pos(record.index);
      read_history.pop();
    }
  }

  /// 直接设置光标位置（如果 new_pos <= data.size()），并更新 current_page
  bool set_cursor_pos(size_t new_pos) {
    if (new_pos <= data.size()) {
      cursor_pos = new_pos;
      update_page();
      return true;
    }
    return false;
  }

  /// 从当前位置向前/向后平移 offset 字节
  bool move(size_t offset) {
    size_t new_pos = cursor_pos + offset;
    if (new_pos <= data.size() && new_pos > 0) {
      cursor_pos = new_pos;
      update_page();
      return true;
    }
    return false;
  }

  /// 光标前移 1 字节
  bool pre() {
    if (cursor_pos > 0) {
      --cursor_pos;
      update_page();
      return true;
    }
    return false;
  }

  /// 光标后移 1 字节
  bool next() {
    if (cursor_pos + 1 < data.size()) {
      ++cursor_pos;
      update_page();
      return true;
    }
    return false;
  }

  /// 向上滚动一行
  bool pre_line() {
    if (cursor_pos >= bytes_per_line) {
      cursor_pos -= bytes_per_line;
      update_page();
      return true;
    }
    return false;
  }

  /// 向下滚动一行
  bool next_line() {
    if (cursor_pos + bytes_per_line < data.size()) {
      cursor_pos += bytes_per_line;
      update_page();
      return true;
    }
    return false;
  }

  /// 更新 current_page = floor(cursor_pos / (bytes_per_line * hex_view_h))
  void update_page() {
    current_page = (cursor_pos / bytes_per_line) / hex_view_h;
  }

  /// 下一页：页号加一，并把 cursor_pos 跳到该页开头
  bool next_page() {
    size_t tp = total_pages();
    if (current_page + 1 < tp) {
      ++current_page;
      cursor_pos = current_page * bytes_per_line * hex_view_h;
      return true;
    }
    return false;
  }

  /// 上一页
  bool pre_page() {
    if (current_page > 0) {
      --current_page;
      cursor_pos = current_page * bytes_per_line * hex_view_h;
      return true;
    }
    return false;
  }

  /// 将 read_history 中所有 Record 按照“最早→最晚”的顺序返回一个 vector
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
  /// 将指针指向的 T 类型的“字节数组”做大小端颠倒
  void reverse_bytes(uint8_t *bytes, size_t size) const {
    for (size_t i = 0; i + 1 < size - i; ++i) {
      std::swap(bytes[i], bytes[size - 1 - i]);
    }
  }
};

// ========== ReaderStrategy 抽象基类 ==========
/// 定义一个虚函数 `bool read(AppState&)`，由子类实现具体读取逻辑
class ReaderStrategy {
public:
  virtual ~ReaderStrategy() = default;
  virtual bool read(AppState &state, std::string type) const = 0;
};

// ========== TypedReader<T> ==========
/// 根据 T 类型来读取一个值，并把读取信息写进 status_msg
template <typename T> class TypedReader : public ReaderStrategy {
public:
  explicit TypedReader(std::string typeName) : typeName_(std::move(typeName)) {}

  bool read(AppState &state, std::string type) const override {
    const size_t orig_pos = state.cursor_pos;
    T value = state.read<T>(orig_pos);
    state.status_msg =
        fmt::format("Read {}: {} @ 0x{:X}", type, Utils::format_value(value),
                    static_cast<unsigned long long>(orig_pos));
    return true;
  }

private:
  std::string typeName_;
};

// ========== FixStringReader ==========
/// 固定长度字符串读取，每次读取长度为 5
class FixStringReader : public ReaderStrategy {
public:
  bool read(AppState &state, std::string type) const override {
    try {
      const size_t orig_pos = state.cursor_pos;
      size_t len = Utils::parse_char_length(type);
      std::string value = state.read_fixed_string(orig_pos, len);
      state.status_msg =
          fmt::format("Read {}: {} @ 0x{:X}", type, Utils::format_value(value),
                      static_cast<unsigned long long>(orig_pos));
      return true;
    } catch (const std::exception &e) {
      state.status_msg = fmt::format("{} read failed: {}", type, e.what());
      return false;
    }
  }
};

// ========== LengthPrefixedStringReader<LengthType> ==========
/// 读取“长度前缀字符串”——先用 LengthType 读取长度，再读取对应字节数的字符串
template <typename LengthType>
class LengthPrefixedStringReader : public ReaderStrategy {
public:
  bool read(AppState &state, std::string type) const override {
    const size_t orig_pos = state.cursor_pos;
    std::string value = state.read_length_prefixed_string<LengthType>(orig_pos);
    state.status_msg =
        fmt::format("Read {}: {} @ 0x{:X}", type, Utils::format_value(value),
                    static_cast<unsigned long long>(orig_pos));
    return true;
  }
};

// ========== ReaderFactory ==========
/// 单例工厂：根据字符串 key (“u8”、“i16”、“string”、“string@u8”等) 选出对应的
/// ReaderStrategy
class ReaderFactory {
public:
  /// 拿到单例引用
  static const ReaderFactory &instance();

  /// 通过 type 字符串去 readers_ map 中找，若存在则执行对应的 read() 并返回
  /// true
  bool read(AppState &state, const std::string &type) const;

private:
  ReaderFactory(); // 构造函数私有
  ReaderFactory(const ReaderFactory &) = delete;
  ReaderFactory &operator=(const ReaderFactory &) = delete;

  template <typename T> void emplaceReader(const std::string &typeName);

  using StrategyMap =
      std::unordered_map<std::string, std::unique_ptr<ReaderStrategy>>;
  StrategyMap readers_;
};
