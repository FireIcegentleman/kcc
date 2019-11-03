//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_ERROR_H_
#define KCC_SRC_ERROR_H_

#include <fmt/color.h>
#include <fmt/core.h>

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "token.h"

namespace kcc {

extern std::vector<std::string> WarningStrs;
void PrintWarnings();

template <typename... Args>
[[noreturn]] void Error(std::string_view format_str, const Args &... args) {
  fmt::print(fmt::fg(fmt::terminal_color::red), "error: ");
  fmt::print(fmt::fg(fmt::terminal_color::red), format_str, args...);
  fmt::print("\n");

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

template <typename... Args>
[[noreturn]] void Error(const SourceLocation &location,
                        std::string_view format_str, const Args &... args) {
  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}:{}:{}: error: "),
             location.file_name, location.row, location.column);
  fmt::print(fmt::fg(fmt::terminal_color::red), format_str, args...);
  fmt::print("\n");

  std::string str;
  for (auto index{location.line_begin};
       index < std::strlen(location.line_content) &&
       location.line_content[index] != '\n';
       ++index) {
    str.push_back(location.line_content[index]);
  }

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}\n"), str);
  fmt::print(fmt::fg(fmt::terminal_color::green), fmt("{}{}\n"),
             std::string(location.column - 1, ' '), "^");

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

template <typename... Args>
void Warning(std::string_view format_str, const Args &... args) {
  std::ostringstream os;

  os << "warning: " << fmt::format(format_str, args...) << '\n';
  WarningStrs.push_back(os.str());
}

template <typename... Args>
void Warning(const SourceLocation &location, std::string_view format_str,
             const Args &... args) {
  std::ostringstream os;

  os << fmt::format(fmt("{}:{}:{}: warning: "), location.file_name,
                    location.row, location.column)
     << fmt::format(format_str, args...) << '\n';

  std::string str;
  for (auto index{location.line_begin};
       index < std::strlen(location.line_content) &&
       location.line_content[index] != '\n';
       ++index) {
    str.push_back(location.line_content[index]);
  }

  os << str << '\n' << std::string(location.column - 1, ' ') << "^\n";
  WarningStrs.push_back(os.str());
}

[[noreturn]] void Error(Tag tag, const Token &actual);

template <typename... Args>
[[noreturn]] void Error(const Token &tok, std::string_view format_str,
                        const Args &... args) {
  Error(tok.GetLoc(), format_str, args...);
}

template <typename... Args>
[[noreturn]] void Warning(const Token &tok, std::string_view format_str,
                          const Args &... args) {
  Warning(tok.GetLoc(), format_str, args...);
}

}  // namespace kcc

#endif  // KCC_SRC_ERROR_H_
