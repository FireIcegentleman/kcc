//
// Created by kaiser on 2019/10/30.
//

#include "cpp.h"

#include <cstddef>
#include <cstdio>
#include <filesystem>

#include "error.h"

namespace kcc {

void EnsureFileExists(const std::string &input_file) {
  if (!std::filesystem::exists(input_file)) {
    Error("no such file: '{}'", input_file);
  }
}

void Preprocessor::SetIncludePaths(
    const std::vector<std::string> &include_paths) {
  for (const auto &item : include_paths) {
    cmd_ += "-I" + item + " ";
  }
}

void Preprocessor::SetMacroDefinitions(
    const std::vector<std::string> &macro_definitions) {
  for (const auto &item : macro_definitions) {
    cmd_ += "-D" + item + " ";
  }
}

std::string Preprocessor::Run(const std::string &input_file) {
  EnsureFileExists(input_file);

  std::string preprocessed_code;

  FILE *fp;
  constexpr std::size_t kSize{4096};
  char buff[kSize];

  if ((fp = popen((cmd_ + input_file).c_str(), "r")) == nullptr) {
    // error
  }

  while (std::fgets(buff, sizeof(buff), fp)) {
    preprocessed_code += buff;
  }

  pclose(fp);

  return preprocessed_code;
}

}  // namespace kcc
