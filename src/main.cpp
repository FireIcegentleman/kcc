//
// Created by kaiser on 2019/10/30.
//

#include <unistd.h>
#include <wait.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include "code_gen.h"
#include "cpp.h"
#include "error.h"
#include "json_gen.h"
#include "lex.h"
#include "link.h"
#include "obj_gen.h"
#include "opt.h"
#include "parse.h"
#include "util.h"

using namespace kcc;

void RunKcc(const std::string &file_name);
#ifdef DEV
void RunTest();
void RunSingle();
void RunDev();
#endif

int main(int argc, char *argv[]) try {
  TimingStart();

  InitCommandLine(argc, argv);
  CommandLineCheck();

#ifdef DEV
  if (TestMode) {
    RunTest();
    return EXIT_SUCCESS;
  } else if (SingleMode) {
    RunSingle();
    return EXIT_SUCCESS;
  } else if (DevMode) {
    RunDev();
    return EXIT_SUCCESS;
  }
#endif

  for (const auto &item : InputFilePaths) {
    auto pid{fork()};
    if (pid < 0) {
      Error("fork error");
    } else if (pid == 0) {
      RunKcc(item);
      PrintWarnings();
      std::exit(EXIT_SUCCESS);
    }
  }

  std::int32_t status{};

  while (waitpid(-1, &status, 0) > 0) {
    // WIFEXITED 如果通过调用 exit 或者 return 正常终止, 则为真
    // WEXITSTATUS 返回一个正常终止的子进程的退出状态, 只有在
    // WIFEXITED 为真时才会定义
    if (!WIFEXITED(status) || WEXITSTATUS(status)) {
      Error("Compile Error");
    }
  }

  if (Preprocess || OutputAssembly || OutputObjectFile || EmitTokens ||
      EmitAST || EmitLLVM) {
    if (Timing) {
      TimingEnd("Timing");
    }
    return EXIT_SUCCESS;
  }

  if (std::empty(OutputFilePath)) {
    OutputFilePath = "a.out";
  }

  auto obj_files{GetObjFiles()};

  if (Static) {
    std::string cmd{"ar qc " + OutputFilePath + ' '};
    for (const auto &item : obj_files) {
      cmd += item + ' ';
    }
    if (!CommandSuccess(std::system(cmd.c_str()))) {
      Error("ar error");
    }

    cmd = "ranlib " + OutputFilePath;
    if (!CommandSuccess(std::system(cmd.c_str()))) {
      Error("ranlib error");
    }

    RemoveAllFiles(RemoveFile);
    if (Timing) {
      TimingEnd("Timing");
    }

    return EXIT_SUCCESS;
  }

  if (!Link(obj_files, OptimizationLevel, OutputFilePath, Shared, RPath, SoFile,
            AFile, Libs)) {
    RemoveAllFiles(RemoveFile);
    Error("Link Failed");
  } else {
    RemoveAllFiles(RemoveFile);
  }

  if (Timing) {
    TimingEnd("Timing");
  }
} catch (const std::exception &error) {
  Error("{}", error.what());
}

void RunKcc(const std::string &file_name) {
  Preprocessor preprocessor;
  preprocessor.SetIncludePaths(IncludePaths);
  preprocessor.SetMacroDefinitions(MacroDefines);

  auto preprocessed_code{preprocessor.Cpp(file_name)};

  if (Preprocess) {
    if (std::empty(OutputFilePath)) {
      std::cout << preprocessed_code << '\n' << std::endl;
    } else {
      std::ofstream ofs{OutputFilePath};
      ofs << preprocessed_code << std::endl;
    }
    return;
  }

  Scanner scanner{std::move(preprocessed_code)};
  auto tokens{scanner.Tokenize()};

  if (EmitTokens) {
    if (std::empty(OutputFilePath)) {
      for (const auto &tok : tokens) {
        if (tok.GetLoc().GetFileName() == file_name) {
          std::cout << tok.ToString() << '\n';
        }
      }
      std::cout << std::endl;
    } else {
      std::ofstream ofs{OutputFilePath};
      for (const auto &tok : tokens) {
        if (tok.GetLoc().GetFileName() == file_name) {
          std::cout << tok.ToString() << '\n';
        }
      }
      std::cout << std::endl;
    }
    return;
  }

  Parser parser{std::move(tokens), file_name};
  auto unit{parser.ParseTranslationUnit()};

  if (EmitAST) {
    JsonGen json_gen{file_name};
    if (std::empty(OutputFilePath)) {
      json_gen.GenJson(unit, GetFileName(file_name, ".html"));
    } else {
      json_gen.GenJson(unit, OutputFilePath);
    }
    return;
  }

  CodeGen code_gen;
  code_gen.GenCode(unit);
  Optimization(OptimizationLevel);

  if (EmitLLVM) {
    std::error_code error_code;
    if (std::empty(OutputFilePath)) {
      llvm::raw_fd_ostream ir_file{GetFileName(file_name, ".ll"), error_code};
      ir_file << *Module;
    } else {
      llvm::raw_fd_ostream ir_file{OutputFilePath, error_code};
      ir_file << *Module;
    }
    return;
  }

  if (OutputAssembly) {
    if (std::empty(OutputFilePath)) {
      ObjGen(GetFileName(file_name, ".s"),
             llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile);
    } else {
      ObjGen(OutputFilePath,
             llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile);
    }
    return;
  }

  if (OutputObjectFile) {
    if (std::empty(OutputFilePath)) {
      ObjGen(GetFileName(file_name, ".o"));
    } else {
      ObjGen(OutputFilePath);
    }
    return;
  }

  ObjGen(GetObjFile(file_name));
}

#ifdef DEV
void Run(const std::string &file) {
  Preprocessor preprocessor;
  preprocessor.SetIncludePaths(IncludePaths);
  preprocessor.SetMacroDefinitions(MacroDefines);

  auto preprocessed_code{preprocessor.Cpp(file)};
  std::ofstream preprocess_file{GetFileName(file, ".i")};
  preprocess_file << preprocessed_code << std::flush;

  Scanner scanner{std::move(preprocessed_code)};
  auto tokens{scanner.Tokenize()};
  std::ofstream tokens_file{GetFileName(file, ".txt")};
  for (const auto &tok : tokens) {
    if (tok.GetLoc().GetFileName() == file) {
      tokens_file << tok.ToString() << '\n';
    }
  }
  tokens_file << std::flush;

  Parser parser{std::move(tokens), file};
  auto unit{parser.ParseTranslationUnit()};
  JsonGen{file}.GenJson(unit, GetFileName(file, ".html"));

  if (StandardIR) {
    std::string cmd{"clang -std=c17 -S -emit-llvm -O0 " + file + " -o " +
                    GetFileName(file, ".std.ll")};
    std::system(cmd.c_str());
  }

  if (!ParseOnly) {
    CodeGen code_gen;
    code_gen.GenCode(unit);

    Optimization(OptimizationLevel);

    std::error_code error_code;
    llvm::raw_fd_ostream ir_file{GetFileName(file, ".ll"), error_code};
    ir_file << *Module;

    std::string cmd{"llc " + GetFileName(file, ".ll")};
    if (!CommandSuccess(std::system(cmd.c_str()))) {
      Error("emit asm fail");
    }

    ObjGen(GetFileName(file, ".o"));
  }
}

void RunTest() {
  Run("testmain.c");

  InputFilePaths.erase(std::remove(std::begin(InputFilePaths),
                                   std::end(InputFilePaths), "testmain.c"),
                       std::end(InputFilePaths));

  std::cout << "Total: " << std::size(InputFilePaths) << '\n';

  for (const auto &file : InputFilePaths) {
    Run(file);

    if (!ParseOnly) {
      EnsureFileExists(GetFileName(file, ".o"));

      if (!Link({GetFileName(file, ".o"), "testmain.o"}, OptimizationLevel,
                GetFileName(file, ".out"))) {
        Error("link fail");
      }

      EnsureFileExists(GetFileName(file, ".out"));
      std::string cmd{"./" + GetFileName(file, ".out")};
      std::system(cmd.c_str());
    }
  }

  PrintWarnings();
}

void RunSingle() {
  for (const auto &file : InputFilePaths) {
    Run(file);
  }

  PrintWarnings();
}

void RunDev() {
  auto file{InputFilePaths.front()};
  Run(file);

  if (!ParseOnly) {
    std::cout << "link\n";
    if (!Link({GetFileName(file, ".o")}, OptimizationLevel,
              GetFileName(file, ".out"))) {
      Error("link fail");
    }
    EnsureFileExists(GetFileName(file, ".out"));

    std::string cmd{"./" + GetFileName(file, ".out")};
    if (!CommandSuccess(std::system(cmd.c_str()))) {
      Error("run fail");
    }

    std::cout << "-----------------------------\n";

    std::cout << "lli\n";
    cmd = "lli " + GetFileName(file, ".ll");
    if (!CommandSuccess(std::system(cmd.c_str()))) {
      Error("run fail");
    }
  }

  PrintWarnings();
}
#endif
