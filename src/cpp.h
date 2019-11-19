//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_CPP_H_
#define KCC_SRC_CPP_H_

#include <string>
#include <vector>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>

namespace kcc {

class Preprocessor {
 public:
  Preprocessor();
  void SetIncludePaths(const std::vector<std::string> &include_paths);
  void SetMacroDefinitions(const std::vector<std::string> &macro_definitions);
  std::string Cpp(const std::string &input_file);

 private:
  clang::CompilerInstance ci_;
  clang::Preprocessor *pp_{};
};

}  // namespace kcc

#endif  // KCC_SRC_CPP_H_
