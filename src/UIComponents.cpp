#include <fmt/format.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "EventHandlers.hpp"
#include "UIComponents.hpp"
#include "Utils.hpp"

using namespace ftxui;

// ========== UI Components ==========
namespace UIComponents {


  Component MainUi(AppState& state, std::string& cmd, ScreenInteractive& screen) {
    // Build each panel
    auto status_bar     = StatusBar(state);
    auto hex_view       = HexView(state);
    auto data_preview   = DataPreviewBar(state);
    auto data_history   = DataReadHistoryBar(state);

    // Command line input with fixed height
    auto command_line = CommandLine(state, cmd) | size(HEIGHT, EQUAL, 3);

    // Arrange hex_view, data_preview, and data_history horizontally
    auto workspace = Container::Horizontal({
        hex_view       | flex | size(WIDTH, EQUAL, 150),
        data_preview   | size(WIDTH, EQUAL, 35),
        data_history   | size(WIDTH, EQUAL, 35),
    }) | flex;

    // Stack status_bar, workspace, and command_line vertically
    auto layout = Container::Vertical({
        status_bar,
        workspace,
        command_line,
    }) | flex;

    // Send an initial focus event so the command line gets focus on startup
    screen.PostEvent(Event::Custom);

    // Wrap with an event catcher to handle custom events, commands, and navigation
    return CatchEvent(layout, [&](const Event& event) {
      // On the first Custom event, set focus to the command line
      if (event == Event::Custom) {
        command_line->TakeFocus();
        return true;
      }

      // Handle user-entered commands
      if (EventHandlers::HandleCommands(state, cmd, screen)(event)) {
        return true;
      }

      // Fallback to navigation (arrow keys, page up/down, etc.)
      return EventHandlers::HandleNavigation(state, screen)(event);
    });
  }

  Component HexView(AppState& state) {
    return Renderer([&] {
      const size_t content_height = state.hex_view_h;
      const size_t start_line     = state.current_page * content_height;
      Elements lines;

      for (size_t i = 0; i < content_height; ++i) {
        const size_t addr = (start_line + i) * state.bytes_per_line;
        if (addr >= state.data.size())
          break;

        Elements hex_cells;
        Elements ascii_cells;

        for (size_t j = 0; j < state.bytes_per_line; ++j) {
          const size_t pos       = addr + j;
          const bool   is_active = (pos == state.cursor_pos);

          if (pos >= state.data.size()) {
            // Past end of data: render spaces
            hex_cells.push_back(text("   "));
            ascii_cells.push_back(text(" "));
            continue;
          }

          const uint8_t byte = state.data[pos];
          // Hex representation (two hex digits + space)
          hex_cells.push_back(
              text(fmt::format("{:02X} ", byte)) |
              (is_active ? inverted : nothing));
          // ASCII representation (printable or dot)
          char disp = (byte >= 0x20 && byte <= 0x7E) ? static_cast<char>(byte) : '.';
          ascii_cells.push_back(
              text(std::string(1, disp)) |
              color(Color::Yellow) |
              (is_active ? inverted : nothing));
        }

        // Combine address, hex cells, separator, and ASCII cells
        lines.push_back(
            hbox({
                text(fmt::format("{:08x}: ", addr)) | color(Color::Blue),
                hbox(hex_cells),
                text("│") | color(Color::GrayDark),
                hbox(ascii_cells),
            }));
      }

      return vbox(lines);
    }) | border;
  }

  Component StatusBar(AppState& state) {
    return Renderer([&] {
      return hbox({
          text(fmt::format(" Pos: 0x{:08x} ", state.cursor_pos)) |
              bgcolor(Color::DarkBlue),
          text(fmt::format(" Page: {}/{} ", state.current_page + 1,
                           state.total_pages())) |
              bgcolor(Color::DarkGreen),
          text(fmt::format(" {} ", state.status_msg)) |
              bgcolor(Color::DarkRed),
          text(fmt::format(" File: {} ", state.file_name)) |
              bgcolor(Color::DarkBlue) | flex,
      });
    });
  }

  Component DataPreviewBar(AppState& state) {
    return Renderer([&] {
      size_t pos        = state.cursor_pos;
      auto   endian_str = state.is_little_endian ? "LE" : "BE";

      // Populate data types with different sizes
      std::vector<std::pair<std::string, std::string>> data_types;
         auto populate = [&](const auto& label, auto type_tag) {
        using T = decltype(type_tag);
        try{
          data_types.emplace_back(label, Utils::format_value(state.peek<T>(pos)));
        } catch (const std::out_of_range& e) {
          data_types.emplace_back(label, "N/A");
        }
      };
      populate("u8", uint8_t{});
      populate("u16", uint16_t{});
      populate("u32", uint32_t{});
      populate("u64", uint64_t{});  
      populate("i8", int8_t{});
      populate("i16", int16_t{});
      populate("i32", int32_t{});
      populate("i64", int64_t{});
      populate("f32", float{});
      populate("f64", double{});

      std::unordered_map<std::string, Color> type_colors = {
            {"u8",  Color::Cyan},
            {"u16", Color::Cyan},
            {"u32", Color::Cyan},
            {"u64", Color::Cyan},

            {"i8",  Color::Yellow},
            {"i16", Color::Yellow},
            {"i32", Color::Yellow},
            {"i64", Color::Yellow},

            {"f32", Color::Magenta},
            {"f64", Color::Magenta},
        };

      Elements data_lines;
      data_lines.push_back(hbox({
              text("Endian:") | bold | color(Color::Green),
              text(endian_str),
          }));
      for (const auto& [type, value] : data_types) {
        Color c = type_colors.find(type) != type_colors.end() ? type_colors[type] : Color::White;
        data_lines.push_back(
            hbox({
                text(fmt::format("{:>3}: ", type)) | color(c),
                text(value) | flex_grow,
            }));
      }

      return vbox(std::move(data_lines)) | border | size(WIDTH, EQUAL, 30);
    });
  }

  Component DataReadHistoryBar(AppState& state) {
    return Renderer([&] {
      // Get the history of read operations
      std::vector<Record> records = state.get_read_history();
      Elements history_lines;

      for (const auto& record : records) {
        history_lines.push_back(
            hbox({
                // Address (in hex)
                text(fmt::format("{:08X}:", record.index)) |
                    color(Color::Green) | flex_shrink,
                // Type name
                text(" " + record.type_name + ": ") | color(Color::Green),
                // Data value
                text(Utils::format_any(record.data)) | flex_grow,
            }));
      }

      return vbox(std::move(history_lines)) | border | size(WIDTH, EQUAL, 30);
    });
  }

  Component CommandLine(AppState& /*_state*/, std::string& command_input) {
    // Configure Input options
    ftxui::InputOption options;
    options.placeholder = "Command (set be/le/pos <num>)";
    options.password    = false;
    options.multiline   = false;
    options.transform   = [](ftxui::InputState state) {
      if (state.is_placeholder) {
        return state.element | color(ftxui::Color::GrayDark);
      }
      return state.element | color(ftxui::Color::White);
    };
    options.on_enter = [] { /* no-op on enter */ };

    // Create the Input component
    auto input = Input(&command_input, options);

    // Render the command line with a prompt and border
    return Renderer(input, [=] {
      return hbox({
                 text(">|") | color(Color::Green),
                 input->Render() | flex,
             }) |
             border | size(HEIGHT, EQUAL, 3);
    });
  }

}  // namespace UIComponents
