//
// Created by kaiser on 2019/11/2.
//

#include "code_gen.h"

#include <llvm/ADT/Optional.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include "error.h"

namespace kcc {

// 拥有许多 LLVM 核心数据结构, 如类型和常量值表
llvm::LLVMContext Context;
// 一个辅助对象, 跟踪当前位置并且可以插入 LLVM 指令
llvm::IRBuilder<> Builder{Context};
// 包含函数和全局变量, 它拥有生成的所有 IR 的内存, 所以使用 llvm::Value*
// 而不使用智能指针
auto Module{std::make_unique<llvm::Module>("main", Context)};

llvm::DataLayout DataLayout{Module.get()};

std::unique_ptr<llvm::TargetMachine> TargetMachine;

CodeGen::CodeGen(const std::string& file_name) {
  Module = std::make_unique<llvm::Module>(file_name, Context);
  DataLayout = llvm::DataLayout{Module.get()};

  // 初始化
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();

  // 获取当前计算机的目标三元组
  auto target_triple{llvm::sys::getDefaultTargetTriple()};

  std::string error;
  auto target{llvm::TargetRegistry::lookupTarget(target_triple, error)};

  if (!target) {
    Error(error);
  }

  // 使用通用CPU, 生成位置无关目标文件
  std::string cpu("generic");
  std::string features;
  llvm::TargetOptions opt;
  llvm::Optional<llvm::Reloc::Model> rm{llvm::Reloc::Model::PIC_};
  TargetMachine = std::unique_ptr<llvm::TargetMachine>{
      target->createTargetMachine(target_triple, cpu, features, opt, rm)};

  // 配置模块以指定目标机器和数据布局, 这不是必需的, 但有利于优化
  Module->setDataLayout(TargetMachine->createDataLayout());
  Module->setTargetTriple(target_triple);
}

void CodeGen::GenCode(const std::shared_ptr<TranslationUnit>& root) {
  (void)result_;
  root->Accept(*this);
}

void CodeGen::Visit(const UnaryOpExpr&) {}

void CodeGen::Visit(const BinaryOpExpr&) {}

void CodeGen::Visit(const ConditionOpExpr&) {}

void CodeGen::Visit(const TypeCastExpr&) {}

void CodeGen::Visit(const Constant&) {}

void CodeGen::Visit(const Enumerator&) {}

void CodeGen::Visit(const CompoundStmt&) {}

void CodeGen::Visit(const IfStmt&) {}

void CodeGen::Visit(const ReturnStmt&) {}

void CodeGen::Visit(const LabelStmt&) {}

void CodeGen::Visit(const FuncCallExpr&) {}

void CodeGen::Visit(const Identifier&) {}

void CodeGen::Visit(const Object&) {}

void CodeGen::Visit(const TranslationUnit& node) {
  for (const auto& item : node.ext_decls_) {
    item->Accept(*this);
  }
}

void CodeGen::Visit(const Declaration& node) {
  auto type{node.GetIdent()->GetType()};

  if (type->IsFunctionTy()) {
    auto func = Module->getFunction(node.GetIdent()->GetName());
    if (!func) {
      llvm::Function::Create(
          llvm::cast<llvm::FunctionType>(type->GetLLVMType()),
          node.GetIdent()->GetLinkage() == kInternal
              ? llvm::Function::InternalLinkage
              : llvm::Function::ExternalLinkage,
          node.GetIdent()->GetName(), Module.get());
    }
  } else {
  }
}

void CodeGen::Visit(const FuncDef&) {}

void CodeGen::Visit(const ExprStmt& node) {
  if (node.expr_) {
    node.expr_->Accept(*this);
  }
}

void CodeGen::Visit(const WhileStmt&) {}

void CodeGen::Visit(const DoWhileStmt&) {}

void CodeGen::Visit(const ForStmt&) {}

void CodeGen::Visit(const CaseStmt&) {}

void CodeGen::Visit(const DefaultStmt&) {}

void CodeGen::Visit(const SwitchStmt&) {}

void CodeGen::Visit(const BreakStmt&) {}

void CodeGen::Visit(const ContinueStmt&) {}

void CodeGen::Visit(const GotoStmt&) {}

}  // namespace kcc
