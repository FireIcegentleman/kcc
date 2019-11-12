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
  clang::Preprocessor *pp_;

  inline static std::string Builtin{
      "typedef struct {\n"
      "  unsigned int gp_offset;\n"
      "  unsigned int fp_offset;\n"
      "  void *overflow_arg_area;\n"
      "  void *reg_save_area;\n"
      "} __va_elem;\n"
      "typedef __va_elem __builtin_va_list[1];\n"
      "void __builtin_va_start(__builtin_va_list, int);\n"
      "void __builtin_va_end(__builtin_va_list);\n"
      "static void *__va_arg_gp(__va_elem *ap) {\n"
      "  void *r = (char *)ap->reg_save_area + ap->gp_offset;\n"
      "  ap->gp_offset += 8;\n"
      "  return r;\n"
      "}\n"
      "static void *__va_arg_fp(__va_elem *ap) {\n"
      "  void *r = (char *)ap->reg_save_area + ap->fp_offset;\n"
      "  ap->fp_offset += 16;\n"
      "  return r;\n"
      "}\n"};
};

}  // namespace kcc

#endif  // KCC_SRC_CPP_H_
