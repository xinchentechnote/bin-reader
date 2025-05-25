#pragma once
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct AppState;
struct ParsedCommand;

void register_all_commands();

using CommandFn = std::function<void(AppState &, const ParsedCommand &)>;

struct ParsedCommand {
  std::string name;
  std::vector<std::string> args;

  static ParsedCommand parse(const std::string &input) {
    ParsedCommand cmd;
    std::istringstream iss(input);
    iss >> cmd.name;

    std::string arg;
    while (iss >> arg)
      cmd.args.push_back(arg);

    return cmd;
  }

  std::string arg(size_t i, const std::string &def = "") const {
    return i < args.size() ? args[i] : def;
  }
};

class CommandRegistry {
public:
  static CommandRegistry &instance() {
    static CommandRegistry inst;
    return inst;
  }

  void register_command(const std::string &name, CommandFn fn) {
    commands_[name] = fn;
  }

  bool dispatch(const ParsedCommand &cmd, AppState &state) const {
    auto it = commands_.find(cmd.name);
    if (it != commands_.end()) {
      it->second(state, cmd);
      return true;
    }
    return false;
  }

private:
  std::unordered_map<std::string, CommandFn> commands_;
};
