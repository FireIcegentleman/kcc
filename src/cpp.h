//
// Created by kaiser on 2019/10/30.
//

#ifndef KCC_SRC_CPP_H_
#define KCC_SRC_CPP_H_

#include <string>
#include <vector>

namespace kcc {

class Preprocessor {
 public:
  void SetIncludePaths(const std::vector<std::string> &include_paths);
  void SetMacroDefinitions(const std::vector<std::string> &macro_definitions);
  std::string Run(const std::string &input_file);

 private:
  std::string cmd_{"gcc -E -std=c17 "};
};

}  // namespace kcc

#endif  // KCC_SRC_CPP_H_
