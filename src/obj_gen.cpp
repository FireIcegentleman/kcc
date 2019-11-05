//
// Created by kaiser on 2019/11/2.
//

#include "obj_gen.h"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include <memory>
#include <system_error>

#include "error.h"

namespace kcc {

extern std::unique_ptr<llvm::Module> Module;

extern std::unique_ptr<llvm::TargetMachine> TargetMachine;

void ObjGen(const std::string& obj_file,
            llvm::TargetMachine::CodeGenFileType file_type) {
  // 初始化
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  std::error_code error_code;
  llvm::raw_fd_ostream dest{obj_file, error_code, llvm::sys::fs::F_None};

  if (error_code) {
    Error("Could not open file: {}", error_code.message());
  }

  // 定义 PassManager 以生成目标代码
  llvm::legacy::PassManager pass;

  if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
    Error("TheTargetMachine can't emit a file of this type");
  }

  pass.run(*Module);
  dest.flush();
}

}  // namespace kcc
