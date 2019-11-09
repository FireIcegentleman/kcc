//
// Created by kaiser on 2019/11/2.
//

#include "location.h"

#include <fmt/core.h>
#include <fmt/format.h>

#include <cstring>

namespace kcc {

std::string Location::ToLocStr() const {
  return fmt::format(fmt("{}:{}:{}:"), file_name, row, column);
}

std::string Location::GetLineContent() const {
  std::string str;

  for (auto index{line_begin};
       index < std::strlen(content) && content[index] != '\n'; ++index) {
    str.push_back(content[index]);
  }

  str += '\n';
  str += fmt::format(fmt("{}{}\n"), std::string(column - 1, ' '), "^");

  return str;
}

}  // namespace kcc
