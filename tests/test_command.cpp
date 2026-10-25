#include "AppState.hpp"
#include "Command.hpp"
#include <gtest/gtest.h>

namespace {

AppState dispatch(const char *input, size_t file_size = 16) {
  static bool registered = false;
  if (!registered) {
    register_all_commands();
    registered = true;
  }
  AppState state;
  state.data.resize(file_size, 0x00);
  state.cursor_pos = 0;
  CommandRegistry::instance().dispatch(ParsedCommand::parse(input), state);
  return state;
}

} // namespace

TEST(ParsedCommandTest, ParseNameAndArgs) {
  const ParsedCommand cmd = ParsedCommand::parse("  r   u32  3 ");
  EXPECT_EQ(cmd.name, "r");
  ASSERT_EQ(cmd.args.size(), 2u);
  EXPECT_EQ(cmd.arg(0), "u32");
  EXPECT_EQ(cmd.arg(1), "3");
  EXPECT_EQ(cmd.arg(2, "-"), "-");
}

TEST(CommandTest, JumpAbsoluteHexAndDecimal) {
  EXPECT_EQ(dispatch("j 0x8").cursor_pos, 8u);
  EXPECT_EQ(dispatch("j 8").cursor_pos, 8u);
  EXPECT_EQ(dispatch("j 0X8").cursor_pos, 8u);
}

// 曾经 "j 0xZZZ" 会被 stoul 静默解析成 0 并跳到文件头
TEST(CommandTest, JumpRejectsMalformedNumbers) {
  AppState state = dispatch("j 0x8");
  state.cursor_pos = 4;
  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("j 0xZZZ"), state);
  EXPECT_EQ(state.cursor_pos, 4u);
  EXPECT_EQ(state.status_msg, "Invalid jump value.");

  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("j 12abc"), state);
  EXPECT_EQ(state.cursor_pos, 4u);
  EXPECT_EQ(state.status_msg, "Invalid jump value.");
}

TEST(CommandTest, JumpOutOfRangeReportsError) {
  AppState state = dispatch("j 0x100");
  EXPECT_EQ(state.cursor_pos, 0u);
  EXPECT_EQ(state.status_msg, "Invalid hex position!");

  state = dispatch("j 16"); // 等于文件大小，仍然越界
  EXPECT_EQ(state.cursor_pos, 0u);
  EXPECT_EQ(state.status_msg, "Invalid absolute position!");
}

// 回归：j -N 必须能回到偏移 0
TEST(CommandTest, JumpRelativeBackwardsReachesZero) {
  AppState state = dispatch("j 0x5");
  ASSERT_EQ(state.cursor_pos, 5u);
  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("j -5"), state);
  EXPECT_EQ(state.cursor_pos, 0u);
  EXPECT_EQ(state.status_msg, "Jumped -5 bytes to 0x0");

  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("j -1"), state);
  EXPECT_EQ(state.cursor_pos, 0u);
  EXPECT_EQ(state.status_msg, "Invalid relative position!");
}

TEST(CommandTest, JumpWithoutArgShowsPosition) {
  AppState state = dispatch("j 0x3");
  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("j"), state);
  EXPECT_EQ(state.status_msg, "Current position: 0x3");
}

TEST(CommandTest, ReadTypeAndFailureMessages) {
  AppState state;
  state.data = {0x01, 0x00, 0x02, 0x00};
  register_all_commands();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("r u16"), state);
  EXPECT_NE(state.status_msg.find("Read u16: 1 "), std::string::npos);
  EXPECT_EQ(state.cursor_pos, 2u);

  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("r bogus"), state);
  EXPECT_EQ(state.status_msg, "Unknown type: bogus");

  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("r"), state);
  EXPECT_NE(state.status_msg.find("Usage: r <type>"), std::string::npos);

  state.cursor_pos = 3; // 只剩 1 字节
  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("r u16"), state);
  EXPECT_NE(state.status_msg.find("u16 read failed"), std::string::npos);
  EXPECT_EQ(state.cursor_pos, 3u);
  EXPECT_EQ(state.get_read_history().size(), 1u); // 失败读取不得新增记录
}

TEST(CommandTest, UndoReportsOffsetOrNothingToUndo) {
  AppState state;
  state.data = {0x01, 0x00, 0x02, 0x00};
  register_all_commands();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("u"), state);
  EXPECT_EQ(state.status_msg, "Nothing to undo.");

  CommandRegistry::instance().dispatch(ParsedCommand::parse("r u16"), state);
  ASSERT_EQ(state.cursor_pos, 2u);
  CommandRegistry::instance().dispatch(ParsedCommand::parse("u"), state);
  EXPECT_EQ(state.status_msg, "Undo to offset: 0x0");
  EXPECT_EQ(state.cursor_pos, 0u);
}

TEST(CommandTest, EndianSwitch) {
  AppState state = dispatch("be");
  EXPECT_FALSE(state.is_little_endian);
  EXPECT_EQ(state.status_msg, "Big-endian mode");
  state.status_msg.clear();
  CommandRegistry::instance().dispatch(ParsedCommand::parse("le"), state);
  EXPECT_TRUE(state.is_little_endian);
  EXPECT_EQ(state.status_msg, "Little-endian mode");
}

TEST(CommandTest, QuitRequestsExit) {
  AppState state = dispatch("q");
  EXPECT_TRUE(state.exit_requested);
  state = dispatch("quit");
  EXPECT_TRUE(state.exit_requested);
}

TEST(CommandTest, UnknownCommandIsNotHandled) {
  AppState state;
  state.data.resize(8);
  register_all_commands();
  EXPECT_FALSE(
      CommandRegistry::instance().dispatch(ParsedCommand::parse("info"), state));
}
