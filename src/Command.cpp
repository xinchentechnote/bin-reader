#include "Command.hpp"
#include "AppState.hpp"
#include <fmt/format.h>

void register_all_commands() {
  CommandRegistry::instance().register_command(
      "u", [](AppState &state, const ParsedCommand &) {
        if (state.undo()) {
          state.status_msg =
              fmt::format("Undo to offset: 0x{:X}", state.cursor_pos);
        } else {
          state.status_msg = "Nothing to undo.";
        }
      });

  CommandRegistry::instance().register_command(
      "be", [](AppState &state, const ParsedCommand &) {
        state.is_little_endian = false;
        state.status_msg = "Big-endian mode";
      });

  CommandRegistry::instance().register_command(
      "le", [](AppState &state, const ParsedCommand &) {
        state.is_little_endian = true;
        state.status_msg = "Little-endian mode";
      });

  CommandRegistry::instance().register_command(
      "j", [](AppState &state, const ParsedCommand &cmd) {
        const std::string sub = cmd.arg(0);
        if (sub.empty()) {
          state.status_msg =
              fmt::format("Current position: 0x{:X}", state.cursor_pos);
          return;
        }

        try {
          if (sub.front() == '+' || sub.front() == '-') {
            size_t consumed = 0;
            const long long delta = std::stoll(sub, &consumed);
            if (consumed != sub.size()) {
              state.status_msg = "Invalid jump value.";
              return;
            }
            state.status_msg = state.move(delta)
                                   ? fmt::format("Jumped {} bytes to 0x{:X}",
                                                 delta, state.cursor_pos)
                                   : "Invalid relative position!";
            return;
          }

          const bool hexadecimal =
              sub.rfind("0x", 0) == 0 || sub.rfind("0X", 0) == 0;
          size_t consumed = 0;
          const size_t pos = static_cast<size_t>(
              std::stoull(sub, &consumed, hexadecimal ? 16 : 10));
          if (consumed != sub.size()) {
            state.status_msg = "Invalid jump value.";
            return;
          }
          if (pos >= state.data.size()) {
            state.status_msg = hexadecimal ? "Invalid hex position!"
                                           : "Invalid absolute position!";
            return;
          }
          state.set_cursor_pos(pos);
          state.status_msg = fmt::format("Position set to 0x{:X}", pos);
        } catch (const std::exception &) {
          state.status_msg = "Invalid jump value.";
        }
      });

  CommandRegistry::instance().register_command(
      "r", [](AppState &state, const ParsedCommand &cmd) {
        if (cmd.args.empty()) {
          state.status_msg =
              "Usage: r <type>  (u8 i8 u16 i16 u32 i32 u64 i64 f32 f64 "
              "char[n] string@u8)";
          return;
        }
        ReaderFactory::instance().read(state, cmd.arg(0));
      });

  CommandRegistry::instance().register_command(
      "quit", [](AppState &state, const ParsedCommand &) {
        state.exit_requested = true;
      });

  CommandRegistry::instance().register_command(
      "q", [](AppState &state, const ParsedCommand &) {
        state.exit_requested = true;
      });
}
