//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_CPP_H_
#define KCC_SRC_CPP_H_

#include <cstddef>
#include <string>
#include <vector>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>

namespace kcc {

class Preprocessor {
 public:
  Preprocessor();
  void SetIncludePaths(const std::vector<std::string> &include_paths);
  // 输入形式为 name=value
  void SetMacroDefinitions(const std::vector<std::string> &macro_definitions);
  std::string Cpp(const std::string &input_file);

 private:
  void AddIncludePath(const std::string &path, bool is_system);

  constexpr static std::size_t kStrReserve{4096};

  clang::CompilerInstance ci_;
  clang::Preprocessor *pp_;
  clang::HeaderSearch *header_search_;
};

}  // namespace kcc

#endif  // KCC_SRC_CPP_H_
