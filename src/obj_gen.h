//
// Created by kaiser on 2019/11/2.
//

#pragma once

#include <string>

#include <llvm/Target/TargetMachine.h>

namespace kcc {

void ObjGen(const std::string &obj_file,
            llvm::TargetMachine::CodeGenFileType file_type =
                llvm::TargetMachine::CodeGenFileType::CGFT_ObjectFile);

}  // namespace kcc
