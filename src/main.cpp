#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <functional>
#include <iterator>

#include "code_gen.h"
#include "cpp.h"
#include "error.h"
#include "json_gen.h"
#include "lex.h"
#include "obj_gen.h"
#include "opt.h"
#include "parse.h"

namespace kcc {

extern std::unique_ptr<llvm::Module> Module;

}  // namespace kcc

using namespace kcc;

int main() {
  Preprocessor preprocessor;
  auto preprocessed_code{preprocessor.Cpp("test/dev/test.c")};
  std::ofstream preprocess{"test/dev/test.i"};
  preprocess << preprocessed_code << std::flush;

  Scanner scanner{std::move(preprocessed_code)};
  auto tokens{scanner.Tokenize()};
  std::ofstream tokens_file{"test/dev/test.txt"};
  std::transform(std::begin(tokens), std::end(tokens),
                 std::ostream_iterator<std::string>{tokens_file, "\n"},
                 std::mem_fn(&Token::ToString));

  Parser parser{std::move(tokens)};
  auto unit{parser.ParseTranslationUnit()};

  JsonGen{}.GenJson(unit, "test/dev/test.html");

  CodeGen{"test/dev/test.c"}.GenCode(unit);
  // Optimization(OptLevel::kO3);

  std::error_code error_code;
  llvm::raw_fd_ostream ir_file{"test/dev/test.ll", error_code};
  ir_file << *Module;

  std::system("llc test/dev/test.ll");
  std::system(
      "clang test/dev/test.c -o test/dev/standard.ll -std=c17 -S -emit-llvm");
  std::system("lli test/dev/test.ll");

  ObjGen("test/dev/test.o");

  std::system("clang test/dev/test.o -o test/dev/test");

  // std::system("./test/dev/test");

  PrintWarnings();
}