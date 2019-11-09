//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_OBJ_GEN_H_
#define KCC_SRC_OBJ_GEN_H_

#include <string>

#include <llvm/Target/TargetMachine.h>

namespace kcc {

void ObjGen(const std::string &obj_file,
            llvm::TargetMachine::CodeGenFileType file_type =
                llvm::TargetMachine::CodeGenFileType::CGFT_ObjectFile);

}  // namespace kcc

#endif  // KCC_SRC_OBJ_GEN_H_
