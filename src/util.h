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

#include "ast.h"
#include "memory_pool.h"
#include "scope.h"
#include "type.h"

namespace kcc {

// 拥有许多 LLVM 核心数据结构, 如类型和常量值表
inline llvm::LLVMContext Context;
// 一个辅助对象, 跟踪当前位置并且可以插入 LLVM 指令
inline llvm::IRBuilder<> Builder{Context};
// 包含函数和全局变量, 它拥有生成的所有 IR 的内存
inline auto Module{std::make_unique<llvm::Module>("main", Context)};

inline llvm::DataLayout DataLayout{Module.get()};

inline std::unique_ptr<llvm::TargetMachine> TargetMachine;

inline MemoryPool<BinaryOpExpr> BinaryOpExprPool;
inline MemoryPool<Enumerator> EnumeratorPool;
inline MemoryPool<Identifier> IdentifierPool;
inline MemoryPool<Constant> ConstantPool;
inline MemoryPool<FuncCallExpr> FuncCallExprPool;
inline MemoryPool<TypeCastExpr> TypeCastExprPool;
inline MemoryPool<ConditionOpExpr> ConditionOpExprPool;
inline MemoryPool<UnaryOpExpr> UnaryOpExprPool;
inline MemoryPool<LabelStmt> LabelStmtPool;
inline MemoryPool<ReturnStmt> ReturnStmtPool;
inline MemoryPool<IfStmt> IfStmtPool;
inline MemoryPool<CompoundStmt> CompoundStmtPool;
inline MemoryPool<Object> ObjectPool;
inline MemoryPool<TranslationUnit> TranslationUnitPool;
inline MemoryPool<Declaration> DeclarationPool;
inline MemoryPool<FuncDef> FuncDefPool;
inline MemoryPool<ExprStmt> ExprStmtPool;
inline MemoryPool<WhileStmt> WhileStmtPool;
inline MemoryPool<DoWhileStmt> DoWhileStmtPool;
inline MemoryPool<ForStmt> ForStmtPool;
inline MemoryPool<CaseStmt> CaseStmtPool;
inline MemoryPool<DefaultStmt> DefaultStmtPool;
inline MemoryPool<SwitchStmt> SwitchStmtPool;
inline MemoryPool<GotoStmt> GotoStmtPool;
inline MemoryPool<ContinueStmt> ContinueStmtPool;
inline MemoryPool<BreakStmt> BreakStmtPool;
inline MemoryPool<VoidType> VoidTypePool;
inline MemoryPool<ArithmeticType> ArithmeticTypePool;
inline MemoryPool<PointerType> PointerTypePool;
inline MemoryPool<ArrayType> ArrayTypePool;
inline MemoryPool<StructType> StructTypePool;
inline MemoryPool<FunctionType> FunctionTypePool;
inline MemoryPool<Scope> ScopePool;

bool CommandSuccess(std::int32_t status);

std::string GetPath();

}  // namespace kcc

#endif  // KCC_SRC_UTIL_H_
