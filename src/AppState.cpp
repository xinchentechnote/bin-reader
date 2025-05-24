#include "AppState.hpp"

#include <fmt/format.h> 
#include <memory>

// —— ReaderFactory 单例实现 —— //
const ReaderFactory &ReaderFactory::instance() {
  static ReaderFactory factory;
  return factory;
}

ReaderFactory::ReaderFactory() {
  // 把各种 TypedReader<T> 和 FixStringReader、LengthPrefixedStringReader 注册到 readers_ 中
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

bool ReaderFactory::read(AppState &state, const std::string &type) const {
  auto it = readers_.find(type);
  if (it != readers_.end()) {
    return it->second->read(state);
  }
  return false;
}

template <typename T>
void ReaderFactory::emplaceReader(const std::string &typeName) {
  readers_.emplace(typeName, std::make_unique<TypedReader<T>>(typeName));
}

// 显式实例化
template void ReaderFactory::emplaceReader<uint8_t>(const std::string &);
template void ReaderFactory::emplaceReader<int8_t>(const std::string &);
template void ReaderFactory::emplaceReader<uint16_t>(const std::string &);
template void ReaderFactory::emplaceReader<int16_t>(const std::string &);
template void ReaderFactory::emplaceReader<uint32_t>(const std::string &);
template void ReaderFactory::emplaceReader<int32_t>(const std::string &);
template void ReaderFactory::emplaceReader<uint64_t>(const std::string &);
template void ReaderFactory::emplaceReader<int64_t>(const std::string &);
template void ReaderFactory::emplaceReader<float>(const std::string &);
template void ReaderFactory::emplaceReader<double>(const std::string &);
