//
// Created by kaiser on 2019/11/17.
//

#ifndef KCC_SRC_LLVM_COMMON_H_
#define KCC_SRC_LLVM_COMMON_H_

#include <cstdint>
#include <memory>
#include <string>

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Target/TargetMachine.h>

#include "ast.h"
#include "type.h"

namespace kcc {

// 拥有许多 LLVM 核心数据结构, 如类型和常量值表
inline llvm::LLVMContext Context;
// 一个辅助对象, 跟踪当前位置并且可以插入 LLVM 指令
inline llvm::IRBuilder<> Builder{Context};
// 包含函数和全局变量, 它拥有生成的所有 IR 的内存
inline std::unique_ptr<llvm::Module> Module;

inline std::unique_ptr<llvm::TargetMachine> TargetMachine;

enum class OptLevel { kO0, kO1, kO2, kO3 };

std::string LLVMTypeToStr(llvm::Type *type);

std::string LLVMConstantToStr(llvm::Constant *type);

llvm::Constant *GetConstantZero(llvm::Type *type);

std::int32_t FloatPointRank(llvm::Type *type);

bool IsArrCastToPtr(llvm::Value *value, llvm::Type *type);

bool IsIntegerTy(llvm::Value *value);

bool IsFloatingPointTy(llvm::Value *value);

bool IsPointerTy(llvm::Value *value);

llvm::Constant *ConstantCastToBool(llvm::Constant *value);

llvm::Constant *ConstantCastTo(llvm::Constant *value, llvm::Type *to,
                               bool is_unsigned);

llvm::ConstantInt *GetInt32Constant(std::int32_t value);

llvm::Value *CastTo(llvm::Value *value, llvm::Type *to, bool is_unsigned);

llvm::Value *CastToBool(llvm::Value *value);

llvm::Value *GetZero(llvm::Type *type);

llvm::GlobalVariable *CreateGlobalCompoundLiteral(QualType type,
                                                  llvm::Constant *init);

llvm::GlobalVariable *CreateGlobalVar(QualType type, llvm::Constant *init,
                                      Linkage linkage, const std::string &name);

}  // namespace kcc

#endif  // KCC_SRC_LLVM_COMMON_H_
