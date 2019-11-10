//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_ERROR_H_
#define KCC_SRC_ERROR_H_

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include "ast.h"
#include "location.h"
#include "token.h"

namespace kcc {

inline std::vector<std::string> WarningStrings;

[[noreturn]] void Error(Tag expect, const Token &actual);
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
[[noreturn]] void Error(const Location &loc, std::string_view format_str,
                        const Args &... args) {
  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}: error: "),
             loc.ToLocStr());
  fmt::print(fmt::fg(fmt::terminal_color::red), format_str, args...);
  fmt::print("\n");

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}"),
             loc.GetLineContent());

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

template <typename... Args>
[[noreturn]] void Error(const Token &tok, std::string_view format_str,
                        const Args &... args) {
  Error(tok.GetLoc(), format_str, args...);
}

template <typename... Args>
[[noreturn]] void Error(Expr *expr, std::string_view format_str,
                        const Args &... args) {
  Error(expr->GetLoc(), format_str, args...);
}

template <typename... Args>
void Warning(std::string_view format_str, const Args &... args) {
  std::string str{"warning: "};

  str += fmt::format(format_str, args...);
  str += '\n';

  WarningStrings.push_back(str);
}

template <typename... Args>
void Warning(const Location &loc, std::string_view format_str,
             const Args &... args) {
  std::string str;

  str += fmt::format(fmt("{}: warning: "), loc.ToLocStr());
  str += fmt::format(format_str, args...);
  str += '\n';
  str += loc.GetLineContent();

  WarningStrings.push_back(str);
}

template <typename... Args>
void Warning(const Token &tok, std::string_view format_str,
             const Args &... args) {
  Warning(tok.GetLoc(), format_str, args...);
}

}  // namespace kcc

#endif  // KCC_SRC_ERROR_H_
