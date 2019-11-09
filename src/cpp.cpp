//
// Created by kaiser on 2019/10/30.
//

#include "cpp.h"

#include <cstddef>
#include <cstdio>

#include "error.h"

namespace kcc {

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

std::string Preprocessor::Cpp(const std::string &input_file) {
  constexpr std::size_t kSize{4096};

  FILE *fp;
  char buff[kSize];
  std::string preprocessed_code;
  preprocessed_code.reserve(kSize);

  if ((fp = popen((cmd_ + input_file).c_str(), "r")) == nullptr) {
    Error("Unable to create pipeline");
  }

  while (std::fgets(buff, sizeof(buff), fp)) {
    preprocessed_code += buff;
  }

  pclose(fp);

  return Preprocessor::Builtin + preprocessed_code;
}

}  // namespace kcc
