#include "AppState.hpp"

#include <fmt/format.h>
#include <memory>

// —— ReaderFactory 单例实现 —— //
const ReaderFactory &ReaderFactory::instance() {
  static ReaderFactory factory;
  return factory;
}

ReaderFactory::ReaderFactory() {
  // 把各种 TypedReader<T> 和 FixStringReader、LengthPrefixedStringReader 注册到
  // readers_ 中
  emplaceReader<uint8_t>();
  emplaceReader<int8_t>();
  emplaceReader<uint16_t>();
  emplaceReader<int16_t>();
  emplaceReader<uint32_t>();
  emplaceReader<int32_t>();
  emplaceReader<uint64_t>();
  emplaceReader<int64_t>();
  emplaceReader<float>();
  emplaceReader<double>();

  readers_.emplace("fixstring", std::make_unique<FixStringReader>());
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

bool ReaderFactory::read(AppState &state, const std::string &type) const {
  auto it = readers_.find(type);
  if (Utils::is_char_array_type(type)) {
    it = readers_.find("fixstring");
  }
  if (it != readers_.end()) {
    return it->second->read(state, type);
  }
  return false;
}

template <typename T> void ReaderFactory::emplaceReader() {
  std::string typeName = TypeFactory::getTypeShortName<T>();
  readers_.emplace(typeName, std::make_unique<TypedReader<T>>(typeName));
}

// 显式实例化
template void ReaderFactory::emplaceReader<uint8_t>();
template void ReaderFactory::emplaceReader<int8_t>();
template void ReaderFactory::emplaceReader<uint16_t>();
template void ReaderFactory::emplaceReader<int16_t>();
template void ReaderFactory::emplaceReader<uint32_t>();
template void ReaderFactory::emplaceReader<int32_t>();
template void ReaderFactory::emplaceReader<uint64_t>();
template void ReaderFactory::emplaceReader<int64_t>();
template void ReaderFactory::emplaceReader<float>();
template void ReaderFactory::emplaceReader<double>();
