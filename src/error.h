//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_ERROR_H_
#define KCC_SRC_ERROR_H_

#include <fmt/color.h>
#include <fmt/core.h>

#include <cstdlib>
#include <cstring>
#include <string_view>

#include "token.h"

namespace kcc {

template <typename... Args>
[[noreturn]] void ErrorReportAndExit(std::string_view format_str,
                                     const Args &... args) {
  fmt::print(stderr, fmt::fg(fmt::terminal_color::red), "error: ");
  fmt::print(stderr, fmt::fg(fmt::terminal_color::red), format_str, args...);
  fmt::print(stderr, "\n");

  std::exit(EXIT_FAILURE);
}

template <typename... Args>
[[noreturn]] void ErrorReportAndExit(const SourceLocation &location,
                                     std::string_view format_str,
                                     const Args &... args) {
  fmt::print(stderr, fmt::fg(fmt::terminal_color::red),
             fmt("{}:{}:{}: error: "), location.file_name, location.row,
             location.column);
  fmt::print(stderr, fmt::fg(fmt::terminal_color::red), format_str, args...);
  fmt::print(stderr, "\n");

  std::string str;
  for (auto index{location.line_begin};
       index < std::strlen(location.line_content) &&
       location.line_content[index] != '\n';
       ++index) {
    str.push_back(location.line_content[index]);
  }

  fmt::print(stderr, fmt::fg(fmt::terminal_color::red), fmt("{}\n"), str);
  fmt::print(stderr, fmt::fg(fmt::terminal_color::green), fmt("{}{}\n"),
             std::string(location.column - 1, ' '), "^");

  std::exit(EXIT_FAILURE);
}

template <typename... Args>
void WarningReport(std::string_view format_str, const Args &... args) {
  fmt::print(fmt::fg(fmt::terminal_color::yellow), "warning: ");
  fmt::print(fmt::fg(fmt::terminal_color::yellow), format_str, args...);
  fmt::print("\n");
}

template <typename... Args>
void WarningReport(const SourceLocation &location, std::string_view format_str,
                   const Args &... args) {
  fmt::print(fmt::fg(fmt::terminal_color::yellow), fmt("{}:{}:{}: warning: "),
             location.file_name, location.row, location.column);
  fmt::print(fmt::fg(fmt::terminal_color::yellow), format_str, args...);
  fmt::print("\n");

  std::string str;
  for (auto index{location.line_begin};
       index < std::strlen(location.line_content) &&
       location.line_content[index] != '\n';
       ++index) {
    str.push_back(location.line_content[index]);
  }

  fmt::print(fmt::fg(fmt::terminal_color::yellow), fmt("{}\n"), str);
  fmt::print(fmt::fg(fmt::terminal_color::green), fmt("{}{}\n"),
             std::string(location.column - 1, ' '), "^");
}

}  // namespace kcc

#endif  // KCC_SRC_ERROR_H_
