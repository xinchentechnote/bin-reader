#pragma once

#include <any>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <iomanip>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Command.hpp"
#include "Utils.hpp"

template <typename T> struct TypeTraits {
  static constexpr const char *name = "unknown";
  static constexpr const char *short_name = "unknown";
};

template <> struct TypeTraits<uint8_t> {
  static constexpr const char *name = "uint8_t";
  static constexpr const char *short_name = "u8";
};

template <> struct TypeTraits<int8_t> {
  static constexpr const char *name = "int8_t";
  static constexpr const char *short_name = "i8";
};

template <> struct TypeTraits<uint16_t> {
  static constexpr const char *name = "uint16_t";
  static constexpr const char *short_name = "u16";
};
template <> struct TypeTraits<int16_t> {
  static constexpr const char *name = "int16_t";
  static constexpr const char *short_name = "i16";
};
template <> struct TypeTraits<uint32_t> {
  static constexpr const char *name = "uint32_t";
  static constexpr const char *short_name = "u32";
};
template <> struct TypeTraits<int32_t> {
  static constexpr const char *name = "int32_t";
  static constexpr const char *short_name = "i32";
};
template <> struct TypeTraits<uint64_t> {
  static constexpr const char *name = "uint64_t";
  static constexpr const char *short_name = "u64";
};
template <> struct TypeTraits<int64_t> {
  static constexpr const char *name = "int64_t";
  static constexpr const char *short_name = "i64";
};
template <> struct TypeTraits<float> {
  static constexpr const char *name = "float";
  static constexpr const char *short_name = "f32";
};
template <> struct TypeTraits<double> {
  static constexpr const char *name = "double";
  static constexpr const char *short_name = "f64";
};

class TypeFactory {
private:
  std::unordered_map<std::string, std::type_index> type_map_;

  template <typename T> void registerType() {
    // Store type_index with proper initialization
    const std::type_index type_idx(typeid(T));
    type_map_.insert_or_assign(TypeTraits<T>::short_name, type_idx);
    type_map_.insert_or_assign(TypeTraits<T>::name, type_idx);
  }

  TypeFactory() {
    // Register basic types
    registerType<uint8_t>();
    registerType<int8_t>();
    registerType<uint16_t>();
    registerType<int16_t>();
    registerType<uint32_t>();
    registerType<int32_t>();
    registerType<uint64_t>();
    registerType<int64_t>();
    registerType<float>();
    registerType<double>();
  }

public:
  TypeFactory(const TypeFactory &) = delete;
  TypeFactory &operator=(const TypeFactory &) = delete;

  static TypeFactory &instance() {
    static TypeFactory inst;
    return inst;
  }

  template <typename T> static std::string getTypeShortName() {
    return TypeTraits<T>::short_name;
  }
};

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
  std::vector<Record> read_history; // 已读取的记录，按读取顺序排列，供 undo 与历史面板使用
  Command last_command;            // 最近一次执行的导航命令

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

  void apply_command(const Command &cmd) {
    switch (cmd.type) {
    case CommandType::MoveUp:
      pre_line();
      break;
    case CommandType::MoveDown:
      next_line();
      break;
    case CommandType::MoveLeft:
      pre();
      break;
    case CommandType::MoveRight:
      next();
      break;
    case CommandType::PageUp:
      pre_page();
      break;
    case CommandType::PageDown:
      next_page();
      break;
    case CommandType::ReadByte:
    case CommandType::None:
    case CommandType::Jump:
      break;
    }
  }

  // —— 通用读取/移动 接口 —— //

  /// 〈peek〉：在 pos 处“窥视”一个 T 类型的数据，但不移动光标、不写历史
  template <typename T> T peek(size_t pos) const {
    if (pos > data.size() || sizeof(T) > data.size() - pos) {
      throw std::out_of_range("Attempt to read beyond data bounds");
    }
    T value;
    std::memcpy(&value, &data[pos], sizeof(T));
    if (!is_little_endian) {
      reverse_bytes(reinterpret_cast<uint8_t *>(&value), sizeof(T));
    }
    return value;
  }

  /// 〈read〉：从当前光标处读取一个 T 类型的数据，写入历史并前移光标
  template <typename T> T read() {
    const size_t pos = cursor_pos;
    T value = peek<T>(pos);
    Record record{pos, value};
    record.type_name = TypeFactory::getTypeShortName<T>();
    read_history.push_back(std::move(record));
    move(static_cast<long long>(sizeof(T)));
    return value;
  }

  /// 从当前光标处读取长度为 n 的定长字符串，写入历史并前移光标
  std::string read_fixed_string(size_t n) {
    const size_t pos = cursor_pos;
    if (pos > data.size() || n > data.size() - pos) {
      throw std::out_of_range("Read operation exceeds data size");
    }
    std::string str(data.begin() + static_cast<long>(pos),
                    data.begin() + static_cast<long>(pos + n));
    Record record{pos, str};
    record.type_name = fmt::format("char[{}]", n);
    read_history.push_back(std::move(record));
    move(static_cast<long long>(n));
    return str;
  }

  /// 从当前光标处读取“长度前缀字符串”：先窥视一个 LengthType 长度，再读取对应
  /// 字节数的字符串。整条记录（前缀 + 内容）合并成一条历史，且失败时不留残留。
  template <typename LengthType> std::string read_length_prefixed_string() {
    const size_t pos = cursor_pos;
    if constexpr (std::is_signed_v<LengthType>) {
      if (peek<LengthType>(pos) < 0) {
        throw std::out_of_range("Negative string length prefix");
      }
    }
    const size_t len = static_cast<size_t>(peek<LengthType>(pos));
    if (len > data.size() - pos - sizeof(LengthType)) {
      throw std::out_of_range("Length-prefixed string exceeds data size");
    }
    const size_t body = pos + sizeof(LengthType);
    std::string str(data.begin() + static_cast<long>(body),
                    data.begin() + static_cast<long>(body + len));
    Record record{pos, str};
    record.type_name =
        fmt::format("string@{}", TypeFactory::getTypeShortName<LengthType>());
    read_history.push_back(std::move(record));
    move(static_cast<long long>(sizeof(LengthType) + len));
    return str;
  }

  /// 撤销最近一次读取：弹出历史末条，并把光标恢复到该记录的起始位置
  bool undo() {
    if (read_history.empty()) {
      return false;
    }
    const size_t target = read_history.back().index;
    read_history.pop_back();
    set_cursor_pos(target);
    return true;
  }

  /// 直接设置光标位置（0 <= new_pos <= data.size()），并更新 current_page
  bool set_cursor_pos(size_t new_pos) {
    if (new_pos <= data.size()) {
      cursor_pos = new_pos;
      update_page();
      return true;
    }
    return false;
  }

  /// 从当前光标处平移 delta 字节（可为负），目标越界则不做任何修改
  bool move(long long delta) {
    const long long target = static_cast<long long>(cursor_pos) + delta;
    if (target < 0 || static_cast<size_t>(target) > data.size()) {
      return false;
    }
    return set_cursor_pos(static_cast<size_t>(target));
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

  /// 历史记录的只读视图，按“最早→最晚”排列。渲染每帧都会调用，因此不做拷贝。
  const std::vector<Record> &get_read_history() const { return read_history; }

private:
  /// 将指针指向的 T 类型的“字节数组”做大小端颠倒
  void reverse_bytes(uint8_t *bytes, size_t size) const {
    for (size_t i = 0; i + 1 < size - i; ++i) {
      std::swap(bytes[i], bytes[size - 1 - i]);
    }
  }
};

// ========== ReaderStrategy 抽象基类 ==========
/// 定义一个虚函数 `bool read(AppState&)`，由子类实现具体读取逻辑。
/// 子类可以直接抛出越界异常，由 ReaderFactory 统一转成状态栏提示。
class ReaderStrategy {
public:
  virtual ~ReaderStrategy() = default;
  virtual bool read(AppState &state, const std::string &type) const = 0;
};

// ========== TypedReader<T> ==========
/// 根据 T 类型来读取一个值，并把读取信息写进 status_msg
template <typename T> class TypedReader : public ReaderStrategy {
public:
  explicit TypedReader(std::string typeName) : typeName_(std::move(typeName)) {}

  bool read(AppState &state, const std::string &type) const override {
    const size_t orig_pos = state.cursor_pos;
    T value = state.read<T>();
    state.status_msg =
        fmt::format("Read {}: {} @ 0x{:X}", type, Utils::format_value(value),
                    static_cast<unsigned long long>(orig_pos));
    return true;
  }

private:
  std::string typeName_;
};

// ========== FixStringReader ==========
/// 固定长度字符串读取，长度取自类型串（如 char[10]）
class FixStringReader : public ReaderStrategy {
public:
  bool read(AppState &state, const std::string &type) const override {
    const size_t orig_pos = state.cursor_pos;
    const size_t len = Utils::parse_char_length(type);
    std::string value = state.read_fixed_string(len);
    state.status_msg =
        fmt::format("Read {}: {} @ 0x{:X}", type, Utils::format_value(value),
                    static_cast<unsigned long long>(orig_pos));
    return true;
  }
};

// ========== LengthPrefixedStringReader<LengthType> ==========
/// 读取“长度前缀字符串”——先窥视 LengthType 长度，再读取对应字节数的字符串
template <typename LengthType>
class LengthPrefixedStringReader : public ReaderStrategy {
public:
  bool read(AppState &state, const std::string &type) const override {
    const size_t orig_pos = state.cursor_pos;
    std::string value = state.read_length_prefixed_string<LengthType>();
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

  template <typename T> void emplaceReader();

  using StrategyMap =
      std::unordered_map<std::string, std::unique_ptr<ReaderStrategy>>;
  StrategyMap readers_;
};
