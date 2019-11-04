#include <fstream>
#include <functional>
#include <iterator>

#include "cpp.h"
#include "error.h"
#include "json_gen.h"
#include "lex.h"
#include "parse.h"

using namespace kcc;

int main() {
  Preprocessor preprocessor;
  auto preprocessed_code{preprocessor.Cpp("test/test.c")};
  std::ofstream preprocess{"test/test.i"};
  preprocess << preprocessed_code << std::flush;

  Scanner scanner{std::move(preprocessed_code)};
  auto tokens{scanner.Tokenize()};
  std::ofstream tokens_file{"test/test.txt"};
  std::transform(std::begin(tokens), std::end(tokens),
                 std::ostream_iterator<std::string>{tokens_file, "\n"},
                 std::mem_fn(&Token::ToString));

  Parser parser{std::move(tokens)};
  auto unit{parser.ParseTranslationUnit()};

  JsonGen json_gen;
  json_gen.GenJson(unit, "test/test.html");

  PrintWarnings();
}