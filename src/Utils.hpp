#pragma once
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fmt/format.h>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "CLI11.hpp"

namespace Utils {
inline std::string ParseCommandLine(int argc, char **argv) {
  CLI::App app{"bin-reader"};
  std::string file_path;

  app.add_option("-f,--file", file_path, "Binary file to load")
      ->required()
      ->check(CLI::ExistingFile);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    std::exit(app.exit(e));
  }

  return file_path;
}

// 数值格式化模板函数：浮点数保留 3 位小数，其余按十进制输出
template <typename T> inline std::string format_value(T value) {
  if constexpr (std::is_floating_point_v<T>) {
    return fmt::format("{:.3f}", value);
  } else {
    return fmt::format("{}", value);
  }
}

inline std::string format_any(const std::any &data) {
  // 有符号整数
  if (typeid(int8_t) == data.type())
    return format_value(std::any_cast<int8_t>(data));
  if (typeid(int16_t) == data.type())
    return format_value(std::any_cast<int16_t>(data));
  if (typeid(int32_t) == data.type())
    return format_value(std::any_cast<int32_t>(data));
  if (typeid(int64_t) == data.type())
    return format_value(std::any_cast<int64_t>(data));

  // 无符号整数
  if (typeid(uint8_t) == data.type())
    return format_value(std::any_cast<uint8_t>(data));
  if (typeid(uint16_t) == data.type())
    return format_value(std::any_cast<uint16_t>(data));
  if (typeid(uint32_t) == data.type())
    return format_value(std::any_cast<uint32_t>(data));
  if (typeid(uint64_t) == data.type())
    return format_value(std::any_cast<uint64_t>(data));

  // 浮点数
  if (typeid(float) == data.type())
    return format_value(std::any_cast<float>(data));
  if (typeid(double) == data.type())
    return format_value(std::any_cast<double>(data));

  // 其他类型
  if (typeid(std::string) == data.type())
    return std::any_cast<std::string>(data);

  return "Unsupported Type";
}

inline std::vector<uint8_t> read_binary_file(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    throw std::runtime_error("Cannot open file: " + path);
  }

  auto file_size = file.tellg();
  if (file_size == -1) {
    throw std::runtime_error("Cannot determine file size");
  }

  file.seekg(0);
  std::vector<uint8_t> data(file_size);

  if (!file.read(reinterpret_cast<char *>(data.data()), file_size)) {
    throw std::runtime_error("File read error");
  }

  return data;
}

inline bool is_char_array_type(const std::string &type) {
  return type.size() > 6 && type.compare(0, 5, "char[") == 0 &&
         type.back() == ']';
}

// 解析 char[10] 中的数字，拒绝非数字字符并防止十进制累加溢出
inline size_t parse_char_length(const std::string &type) {
  size_t len = 0;
  for (size_t i = 5; i + 1 < type.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(type[i]))) {
      throw std::invalid_argument("Non-digit in char[] length");
    }
    if (len > (std::numeric_limits<size_t>::max() - 9) / 10) {
      throw std::out_of_range("char[] length overflow");
    }
    len = len * 10 + static_cast<size_t>(type[i] - '0');
  }
  return len;
}
} // namespace Utils