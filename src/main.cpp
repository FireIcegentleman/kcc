//
// Created by kaiser on 2019/10/30.
//

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>

#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include "code_gen.h"
#include "cpp.h"
#include "error.h"
#include "json_gen.h"
#include "lex.h"
#include "obj_gen.h"
#include "opt.h"
#include "parse.h"
#include "util.h"

using namespace kcc;

llvm::cl::OptionCategory Category{"Compiler Options"};

llvm::cl::list<std::string> InputFilePaths{
    llvm::cl::desc{"files"}, llvm::cl::Positional, llvm::cl::cat{Category}};

llvm::cl::opt<bool> OutputObjectFile{
    "c", llvm::cl::desc{"Only run preprocess, compile, and assemble steps"},
    llvm::cl::cat{Category}};

llvm::cl::list<std::string> MacroDefines{
    "D", llvm::cl::desc{"Define <macro> to <value> (or 1 if <value> omitted)"},
    llvm::cl::value_desc{"macro>=<value"}, llvm::cl::Prefix,
    llvm::cl::cat{Category}};

llvm::cl::opt<bool> EmitAst{
    "emit-ast", llvm::cl::desc{"Emit tcc AST files for source inputs"},
    llvm::cl::cat{Category}};

llvm::cl::opt<bool> EmitLlvm{
    "emit-llvm",
    llvm::cl::desc{
        "Use the LLVM representation for assembler and object files"},
    llvm::cl::cat{Category}};

llvm::cl::opt<bool> Preprocess{"E", llvm::cl::desc{"Only run the preprocessor"},
                               llvm::cl::cat{Category}};

llvm::cl::list<std::string> IncludePaths{
    "I", llvm::cl::desc{"Add directory to include search path"},
    llvm::cl::value_desc{"dir"}, llvm::cl::Prefix, llvm::cl::cat{Category}};

llvm::cl::opt<std::string> OutputFilePath{
    "o", llvm::cl::desc("Write output to <file>"), llvm::cl::value_desc("file"),
    llvm::cl::cat{Category}};

llvm::cl::opt<OptLevel> OptimizationLevel{
    llvm::cl::desc{"Set optimization level to <number>"},
    llvm::cl::value_desc{"number"}, llvm::cl::init(OptLevel::kO2),
    llvm::cl::values(
        clEnumValN(OptLevel::kO0, "O0", "No optimizations"),
        clEnumValN(OptLevel::kO1, "O1", "Enable trivial optimizations"),
        clEnumValN(OptLevel::kO2, "O2", "Enable default optimizations"),
        clEnumValN(OptLevel::kO3, "O3", "Enable expensive optimizations")),
    llvm::cl::cat{Category}};

llvm::cl::opt<bool> OutputAssembly{
    "S", llvm::cl::desc{"Only run preprocess and compilation steps"},
    llvm::cl::cat{Category}};

llvm::cl::opt<bool> DevMode{"dev", llvm::cl::desc{"Dev Mode"},
                            llvm::cl::cat{Category}};

void RunDev();

int main(int argc, char *argv[]) /*try*/ {
  llvm::cl::HideUnrelatedOptions(Category);

  llvm::cl::SetVersionPrinter([](llvm::raw_ostream &os) {
    os << "C Compiler by Kaiser\n";
    os << "Version " << KCC_VERSION << '\n';
    os << "InstalledDir: " << GetPath() << '\n';
  });

  llvm::cl::ParseCommandLineOptions(argc, argv);

  if (DevMode) {
    RunDev();
  }

  std::vector<std::string> input_file_paths_checked;

  for (const auto &item : InputFilePaths) {
    std::filesystem::path path{item};

    if (std::filesystem::is_directory(path)) {
      Error("Can't be a directory: {}", path.string());
    }

    if (path.filename().string() == "*.c") {
      for (const auto &file :
           std::filesystem::directory_iterator{path.parent_path()}) {
        if (file.path().filename().extension().string() == ".c") {
          input_file_paths_checked.push_back(file.path().string());
        }
      }
    } else if (std::filesystem::exists(path)) {
      if (path.filename().extension().string() != ".c") {
        Error("the file extension must be '.c': {}", item);
      }
      input_file_paths_checked.push_back(item);
    } else {
      Error("no such file: {}", item);
    }
  }

  if (std::size(input_file_paths_checked) == 0) {
    Error("no input files");
  }

  for (const auto &folder : IncludePaths) {
    if (!std::filesystem::exists(folder)) {
      Error("no such directory: {}", folder);
    }
  }

  if (!std::empty(OutputFilePath) && std::size(input_file_paths_checked) > 1 &&
      (Preprocess || OutputAssembly || OutputObjectFile || EmitAst ||
       EmitLlvm)) {
    Error("Cannot specify -o when generating multiple output files");
  }

  Preprocessor pre;
  pre.SetIncludePaths(IncludePaths);
  pre.SetMacroDefinitions(MacroDefines);

  if (Preprocess) {
    if (std::empty(OutputFilePath)) {
      for (const auto &file_name : input_file_paths_checked) {
        std::cout << pre.Cpp(file_name) << "\n\n";
      }
    } else {
      std::ofstream ofs{OutputFilePath};
      ofs << pre.Cpp(input_file_paths_checked.front());
    }
  } else if (EmitAst) {
    for (const auto &file : input_file_paths_checked) {
      Scanner scanner{pre.Cpp(file)};

      Parser parser{scanner.Tokenize()};
      auto root{parser.ParseTranslationUnit()};

      JsonGen json_gen;

      if (std::empty(OutputFilePath)) {
        json_gen.GenJson(
            root,
            std::filesystem::path{file}.filename().stem().string() + ".html");
      } else {
        json_gen.GenJson(root, OutputFilePath);
      }
    }
  } else if (EmitLlvm) {
    for (const auto &file : input_file_paths_checked) {
      Scanner scanner{pre.Cpp(file)};

      Parser parser{scanner.Tokenize()};
      auto root{parser.ParseTranslationUnit()};

      CodeGen context{file};
      context.GenCode(root);

      Optimization(OptimizationLevel);

      std::error_code error_code;

      if (std::empty(OutputFilePath)) {
        llvm::raw_fd_ostream ir_file{
            std::filesystem::path{file}.filename().stem().string() + ".ll",
            error_code};
        ir_file << *Module;
      } else {
        llvm::raw_fd_ostream ir_file{OutputFilePath, error_code};
        ir_file << *Module;
      }
    }
  } else if (OutputAssembly) {
    for (const auto &file_name : input_file_paths_checked) {
      Scanner scanner{pre.Cpp(file_name)};

      Parser parser{scanner.Tokenize()};
      auto root{parser.ParseTranslationUnit()};

      CodeGen context{file_name};
      context.GenCode(root);

      Optimization(OptimizationLevel);

      if (std::empty(OutputFilePath)) {
        ObjGen(
            std::filesystem::path{file_name}.filename().stem().string() + ".s",
            llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile);
      } else {
        ObjGen(OutputFilePath,
               llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile);
      }
    }
  } else if (OutputObjectFile) {
    for (const auto &file_name : input_file_paths_checked) {
      Scanner scanner{pre.Cpp(file_name)};

      Parser parser{scanner.Tokenize()};
      auto root{parser.ParseTranslationUnit()};

      CodeGen context{file_name};
      context.GenCode(root);

      Optimization(OptimizationLevel);

      if (std::empty(OutputFilePath)) {
        ObjGen(std::filesystem::path{file_name}.filename().stem().string() +
               ".o");
      } else {
        ObjGen(OutputFilePath);
      }
    }
  } else {
    std::ostringstream obj_files;
    std::vector<std::string> files_to_delete;
    std::default_random_engine e{std::random_device{}()};

    auto curr_path{std::filesystem::current_path()};

    for (const auto &file : input_file_paths_checked) {
      Scanner scanner{pre.Cpp(file)};

      Parser parser{scanner.Tokenize()};
      auto root{parser.ParseTranslationUnit()};

      CodeGen context{file};
      context.GenCode(root);

      Optimization(OptimizationLevel);

      auto obj_file{std::filesystem::path{file}.filename().stem().string() +
                    std::to_string(e()) + ".o"};

      obj_files << (curr_path / obj_file).string() << ' ';
      files_to_delete.push_back(obj_file);

      ObjGen(obj_file);
    }

    if (std::empty(OutputFilePath)) {
      OutputFilePath = "a.out";
    }

    std::string cmd("clang -o " +
                    (curr_path / OutputFilePath.c_str()).string() + ' ' +
                    obj_files.str());

    if (!CommandSuccess(std::system(cmd.c_str()))) {
      for (const auto &file : files_to_delete) {
        std::filesystem::remove(file);
      }

      Error("Link Failed");
    }

    for (const auto &file : files_to_delete) {
      std::filesystem::remove(file);
    }
  }
}
// catch (const std::exception &err) {
//  Error(err.what());
//}

void RunDev() {
  Preprocessor preprocessor;
  std::string preprocessed_code;
  {
    std::cout << "cpp ............................ ";
    const auto kT0{std::chrono::system_clock::now()};
    preprocessed_code = preprocessor.Cpp("test/dev/test.c");
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::system_clock::now() - kT0)
                     .count()
              << " us\n";
  }
  std::ofstream preprocess_file{"test/dev/test.i"};
  preprocess_file << preprocessed_code << std::flush;

  Scanner scanner{std::move(preprocessed_code)};
  std::vector<Token> tokens;
  {
    std::cout << "lex ............................ ";
    const auto kT0{std::chrono::system_clock::now()};
    tokens = scanner.Tokenize();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::system_clock::now() - kT0)
                     .count()
              << " us\n";
  }
  std::ofstream tokens_file{"test/dev/test.txt"};
  std::transform(std::begin(tokens), std::end(tokens),
                 std::ostream_iterator<std::string>{tokens_file, "\n"},
                 std::mem_fn(&Token::ToString));
  tokens_file << std::flush;

  //  Parser parser{std::move(tokens)};
  //  TranslationUnit *unit;
  //  {
  //    std::cout << "parse ............................ ";
  //    const auto kT0{std::chrono::system_clock::now()};
  //    unit = parser.ParseTranslationUnit();
  //    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(
  //                     std::chrono::system_clock::now() - kT0)
  //                     .count()
  //              << " us\n";
  //  }
  //  JsonGen{}.GenJson(unit, "test/dev/test.html");
  //  {
  //    std::cout << "code gen ............................ ";
  //    const auto kT0{std::chrono::system_clock::now()};
  //    CodeGen{"test/dev/test.c"}.GenCode(unit);
  //    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(
  //                     std::chrono::system_clock::now() - kT0)
  //                     .count()
  //              << " us\n";
  //  }
  //  Optimization(OptLevel::kO0);
  //
  //  std::error_code error_code;
  //  llvm::raw_fd_ostream ir_file{"test/dev/test.ll", error_code};
  //  ir_file << *Module;
  //
  //  std::system("llc test/dev/test.ll");
  //  std::system(
  //      "clang test/dev/test.c -o test/dev/standard.ll -std=c17 -S
  //      -emit-llvm");
  //  std::system("lli test/dev/test.ll");
  //
  //  ObjGen("test/dev/test.o");
  //
  //  std::system("clang test/dev/test.o -o test/dev/test");

  // std::system("./test/dev/test");

  PrintWarnings();

  std::exit(EXIT_SUCCESS);
}
