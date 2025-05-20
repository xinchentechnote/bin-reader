cmake_minimum_required(VERSION 3.15)
project(bin-reader
  LANGUAGES CXX
  VERSION 1.0.0
)

#----------------------- 全局设置 --------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置构建输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 编译器警告设置
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

#--------------------- 依赖管理配置 ------------------------
# 设置依赖缓存到项目构建目录
# 设置 FetchContent 的全局缓存目录为 ~/_deps
set(FETCHCONTENT_BASE_DIR "$ENV{HOME}/_deps" CACHE PATH "Base directory for FetchContent downloads")
# set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps")
file(MAKE_DIRECTORY ${FETCHCONTENT_BASE_DIR})

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)  # 重用已下载依赖

#--------------------- 第三方依赖配置 ----------------------
# FTXUI (终端UI库)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG        v6.1.8
  GIT_PROGRESS ON
  GIT_SHALLOW    TRUE
)

# fmt (格式化库)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        8.1.1
  GIT_PROGRESS ON
  GIT_SHALLOW    TRUE
)

# Catch2 (单元测试框架)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
  GIT_PROGRESS ON
  GIT_SHALLOW    TRUE
)

# 并行获取所有依赖
FetchContent_MakeAvailable(ftxui fmt Catch2)

#--------------------- 主程序目标配置 ----------------------
add_executable(bin-reader
  src/main.cpp
  src/model/BinaryEditor.cpp
  src/controller/EditorController.cpp
  src/view/EditorView.cpp
)

target_include_directories(bin-reader PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(bin-reader PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
  fmt::fmt
)

#--------------------- 单元测试配置 ------------------------
if(BUILD_TESTING)
  enable_testing()

  add_executable(bin-reader-tests
    tests/test_main.cpp
    src/model/BinaryEditor.cpp  # 包含实现文件
  )

  target_include_directories(bin-reader-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(bin-reader-tests PRIVATE
    Catch2::Catch2WithMain
    fmt::fmt  # 测试可能需要格式化输出
  )

  # 自动发现测试用例
  include(CTest)
  include(Catch)
  catch_discover_tests(bin-reader-tests)
endif()