#pragma once
#include <fmt/format.h>
#include <type_traits>

namespace Utils {
// 数值格式化模板函数
template <typename T> inline std::string format_value(T value) {
  if constexpr (std::is_floating_point_v<T>) {
    // 浮点数保留3位小数
    return fmt::format("{:.3f}", value);
  } else if constexpr (std::is_signed_v<T>) {
    // 有符号整数显示十进制和十六进制
    return fmt::format("{}",
                       value); // 按字节数确定十六进制位数
  } else {
    // 无符号整数显示十进制和十六进制
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
} // namespace Utils