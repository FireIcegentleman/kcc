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

  CodeGen{}.GenCode(unit);
  Optimization(OptLevel::kO3);

  std::error_code error_code;
  llvm::raw_fd_ostream ir_file{"test/dev/test.ll", error_code};
  ir_file << *Module;

  ObjGen("test/dev/test.o");

  std::system("gcc test/dev/test.o -c test/dev/test");

  std::system("./test/dev/test");

  PrintWarnings();
}