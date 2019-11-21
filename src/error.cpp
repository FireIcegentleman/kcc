//
// Created by kaiser on 2019/10/31.
//

#include "error.h"

namespace kcc {

void PrintWarnings() {
  for (const auto &[item, arrow] : WarningStrings) {
    fmt::print(fmt::fg(fmt::terminal_color::white), fmt("{}"), item);
    if (!std::empty(arrow)) {
      fmt::print(fmt::fg(fmt::terminal_color::green), fmt("{}"), arrow);
    }
  }
}

[[noreturn]] void Error(Tag tag, const Token &actual) {
  auto loc{actual.GetLoc()};
  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}: error: "),
             loc.ToLocStr());
  fmt::print(fmt::fg(fmt::terminal_color::red), "expected {}, but got {}\n",
             TokenTag::ToString(tag), TokenTag::ToString(actual.GetTag()));

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}"),
             loc.GetLineContent());
  fmt::print(fmt::fg(fmt::terminal_color::green), fmt("{}"),
             loc.GetPositionArrow());

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

}  // namespace kcc
