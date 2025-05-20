cmake_minimum_required(VERSION 3.15)
project(bin-reader CXX)

# 设置 FetchContent 的全局缓存目录为 ~/_deps
set(FETCHCONTENT_BASE_DIR "$ENV{HOME}/_deps" CACHE PATH "Base directory for FetchContent downloads")
# 确保目录存在
file(MAKE_DIRECTORY ${FETCHCONTENT_BASE_DIR})

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
# ui库
set(FTXUI_REPO "git@github.com:ArthurSonzogni/FTXUI.git")
message(STATUS "Fetch FTXUI: ${FTXUI_REPO}")
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY ${FTXUI_REPO}
  GIT_TAG v6.1.8
)
FetchContent_MakeAvailable(ftxui)
#fmt
set(FMT_REPO "git@github.com:fmtlib/fmt.git")
message(STATUS "Fetch fmt: ${FMT_REPO}")
FetchContent_Declare(
  fmt
  GIT_REPOSITORY ${FMT_REPO}
  GIT_TAG 8.1.1
)
FetchContent_MakeAvailable(fmt)

#单元测试
set(CATCH2_REPO "git@github.com:catchorg/Catch2.git")
message(STATUS "Fetch Catch2: ${CATCH2_REPO}")
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY ${CATCH2_REPO}
  GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

enable_testing()

add_executable(tests
    tests/test_main.cpp
)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
include(CTest)
include(Catch)
catch_discover_tests(tests)


add_executable(bin-reader src/main.cpp)
target_link_libraries(bin-reader PRIVATE ftxui::screen ftxui::dom fmt::fmt)
