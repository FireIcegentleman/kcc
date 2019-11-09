//
// Created by kaiser on 2019/11/9.
//

#ifndef KCC_SRC_UTIL_H_
#define KCC_SRC_UTIL_H_

#include <cstdint>
#include <memory>
#include <string>

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

namespace kcc {

// 拥有许多 LLVM 核心数据结构, 如类型和常量值表
inline llvm::LLVMContext Context;
// 一个辅助对象, 跟踪当前位置并且可以插入 LLVM 指令
inline llvm::IRBuilder<> Builder{Context};
// 包含函数和全局变量, 它拥有生成的所有 IR 的内存
inline auto Module{std::make_unique<llvm::Module>("main", Context)};

inline llvm::DataLayout DataLayout{Module.get()};

inline std::unique_ptr<llvm::TargetMachine> TargetMachine;

bool CommandSuccess(std::int32_t status);

std::string GetPath();

}  // namespace kcc

#endif  // KCC_SRC_UTIL_H_
