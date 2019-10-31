//
// Created by kaiser on 2019/10/31.
//

#include "error.h"

namespace kcc {

std::vector<std::string> WarningStrs;

void PrintWarnings() {
  for (const auto &item : WarningStrs) {
    fmt::print(fmt::fg(fmt::terminal_color::yellow), fmt("{}"), item);
  }
}

[[noreturn]] void Error(Tag tag, const Token &actual) {
  auto loc{actual.GetLoc()};
  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}:{}:{}: error: "),
             loc.file_name, loc.row, loc.column);
  fmt::print(fmt::fg(fmt::terminal_color::red), "expected {}, but got {}",
             ToString(tag), ToString(actual.GetTag()));
  fmt::print("\n");

  std::string str;
  for (auto index{loc.line_begin};
       index < std::strlen(loc.line_content) && loc.line_content[index] != '\n';
       ++index) {
    str.push_back(loc.line_content[index]);
  }

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}\n"), str);
  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}{}\n"),
             std::string(loc.column - 1, ' '), "^");

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

}  // namespace kcc
