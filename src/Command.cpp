#include "Command.hpp"
#include "AppState.hpp"
#include <fmt/format.h>

void register_all_commands() {
  CommandRegistry::instance().register_command(
      "u", [](AppState &state, const ParsedCommand &) { state.undo(); });

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
        try {
          std::string sub = cmd.arg(0);
          if (sub.empty()) {
            state.status_msg =
                fmt::format("Current position: 0x{:X}", state.cursor_pos);
          } else if (sub[0] == '+' || sub[0] == '-') {
            long offset = std::stol(sub);
            if (state.move(static_cast<size_t>(offset)))
              state.status_msg = fmt::format("Jumped {} bytes to 0x{:X}",
                                             offset, state.cursor_pos);
            else
              state.status_msg = "Invalid relative position!";
          } else {
            size_t pos = sub.rfind("0x", 0) == 0 ? std::stoul(sub, nullptr, 16)
                                                 : std::stoul(sub);
            if (pos < state.data.size()) {
              state.set_cursor_pos(pos);
              state.status_msg = fmt::format("Position set to 0x{:X}", pos);
            } else {
              state.status_msg = "Invalid absolute position!";
            }
          }
        } catch (...) {
          state.status_msg = "Invalid jump value.";
        }
      });

  CommandRegistry::instance().register_command(
      "r", [](AppState &state, const ParsedCommand &cmd) {
        try {
          ReaderFactory::instance().read(state, cmd.arg(0));
        } catch (...) {
          state.status_msg = "Read failed.";
        }
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
