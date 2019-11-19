//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_CODE_GEN_H_
#define KCC_SRC_CODE_GEN_H_

#include <cstdint>
#include <map>
#include <stack>
#include <string>
#include <utility>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "ast.h"
#include "visitor.h"

namespace kcc {

class CodeGen : public Visitor {
 public:
  CodeGen(const std::string &file_name);
  void GenCode(const TranslationUnit *root);

 private:
  // 在栈上分配内存
  llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *parent,
                                           llvm::Type *type,
                                           std::int32_t align);
  llvm::Value *AddOp(llvm::Value *lhs, llvm::Value *rhs, bool is_unsigned);
  llvm::Value *SubOp(llvm::Value *lhs, llvm::Value *rhs, bool is_unsigned);
  llvm::Value *MulOp(llvm::Value *lhs, llvm::Value *rhs, bool is_unsigned);
  llvm::Value *DivOp(llvm::Value *lhs, llvm::Value *rhs, bool is_unsigned);
  llvm::Value *ModOp(llvm::Value *lhs, llvm::Value *rhs, bool is_unsigned);
  llvm::Value *OrOp(llvm::Value *lhs, llvm::Value *rhs);
  llvm::Value *AndOp(llvm::Value *lhs, llvm::Value *rhs);
  llvm::Value *XorOp(llvm::Value *lhs, llvm::Value *rhs);
  llvm::Value *ShlOp(llvm::Value *lhs, llvm::Value *rhs);
  llvm::Value *ShrOp(llvm::Value *lhs, llvm::Value *rhs, bool is_unsigned);
  llvm::Value *LessEqualOp(llvm::Value *lhs, llvm::Value *rhs,
                           bool is_unsigned);
  llvm::Value *LessOp(llvm::Value *lhs, llvm::Value *rhs, bool is_unsigned);
  llvm::Value *GreaterEqualOp(llvm::Value *lhs, llvm::Value *rhs,
                              bool is_unsigned);
  llvm::Value *GreaterOp(llvm::Value *lhs, llvm::Value *rhs, bool is_unsigned);
  llvm::Value *EqualOp(llvm::Value *lhs, llvm::Value *rhs);
  llvm::Value *NotEqualOp(llvm::Value *lhs, llvm::Value *rhs);

  llvm::Value *CastTo(llvm::Value *value, llvm::Type *to, bool is_unsigned);
  llvm::Value *CastToBool(llvm::Value *value);
  bool IsArithmeticTy(llvm::Value *value) const;
  bool IsIntegerTy(llvm::Value *value) const;
  bool IsFloatingPointTy(llvm::Value *value) const;
  bool IsPointerTy(llvm::Value *value) const;
  bool IsFloatTy(llvm::Value *value) const;
  bool IsDoubleTy(llvm::Value *value) const;
  bool IsLongDoubleTy(llvm::Value *value) const;
  llvm::Value *GetZero(llvm::Type *type);
  std::int32_t FloatPointRank(llvm::Type *type) const;
  llvm::Value *LogicOrOp(const BinaryOpExpr &node);
  llvm::Value *LogicAndOp(const BinaryOpExpr &node);
  llvm::Value *AssignOp(const BinaryOpExpr &node);
  llvm::Value *GetPtr(const AstNode &node);
  llvm::Value *Assign(llvm::Value *lhs_ptr, llvm::Value *rhs,
                      std::int32_t align);
  void VisitForNoInc(const ForStmt &node);
  void VisitForNoCond(const ForStmt &node);
  void VisitForNoIncCond(const ForStmt &node);
  llvm::Value *NegOp(llvm::Value *value, bool is_unsigned);
  llvm::Value *LogicNotOp(llvm::Value *value);
  std::string LLVMTypeToStr(llvm::Type *type) const;
  llvm::Value *IncOrDec(const Expr &expr, bool is_inc, bool is_postfix);
  bool IsArrCastToPtr(llvm::Value *value, llvm::Type *type);
  void EnterFunc();
  void ExitFunc();

  virtual void Visit(const UnaryOpExpr &node) override;
  virtual void Visit(const TypeCastExpr &node) override;
  virtual void Visit(const BinaryOpExpr &node) override;
  virtual void Visit(const ConditionOpExpr &node) override;
  virtual void Visit(const FuncCallExpr &node) override;
  virtual void Visit(const ConstantExpr &node) override;
  virtual void Visit(const StringLiteralExpr &node) override;
  virtual void Visit(const IdentifierExpr &node) override;
  virtual void Visit(const EnumeratorExpr &node) override;
  virtual void Visit(const ObjectExpr &node) override;
  virtual void Visit(const StmtExpr &node) override;

  virtual void Visit(const LabelStmt &node) override;
  virtual void Visit(const CaseStmt &node) override;
  virtual void Visit(const DefaultStmt &node) override;
  virtual void Visit(const CompoundStmt &node) override;
  virtual void Visit(const ExprStmt &node) override;
  virtual void Visit(const IfStmt &node) override;
  virtual void Visit(const SwitchStmt &node) override;
  virtual void Visit(const WhileStmt &node) override;
  virtual void Visit(const DoWhileStmt &node) override;
  virtual void Visit(const ForStmt &node) override;
  virtual void Visit(const GotoStmt &node) override;
  virtual void Visit(const ContinueStmt &node) override;
  virtual void Visit(const BreakStmt &node) override;
  virtual void Visit(const ReturnStmt &node) override;

  virtual void Visit(const TranslationUnit &node) override;
  virtual void Visit(const Declaration &node) override;
  virtual void Visit(const FuncDef &node) override;

  void DealGlobalDecl(const Declaration &node);
  void DealLocaleDecl(const Declaration &node);
  void InitLocalAggregate(const Declaration &node);

  void PushBlock(llvm::BasicBlock *continue_block,
                 llvm::BasicBlock *break_stack);
  void PopBlock();
  bool HasBrOrReturn() const;
  bool HasReturn() const;

  bool MayCallBuiltinFunc(const FuncCallExpr &node);
  llvm::Value *VaArg(llvm::Value *ptr, llvm::Type *type);

  llvm::Value *result_{};
  std::int32_t align_{};

  std::stack<llvm::BasicBlock *> continue_stack_;
  std::stack<llvm::BasicBlock *> break_stack_;
  std::stack<std::pair<bool, bool>> has_br_or_return_;

  std::map<llvm::Constant *, llvm::Constant *> strings_;

  llvm::Function *va_start_{};
  llvm::Function *va_end_{};
  llvm::Function *va_copy_{};

  llvm::BasicBlock *last_{nullptr};

  bool load_{false};

  std::stack<llvm::SwitchInst *> switch_insts_;
  std::stack<llvm::BasicBlock *> switch_after_;
  bool case_has_break_{false};

  std::int32_t alloc_count_{};
  std::vector<std::tuple<std::string, llvm::BasicBlock *, std::int32_t>>
      goto_stmt_;
  std::vector<LabelStmt> labels_;
};

}  // namespace kcc

#endif  // KCC_SRC_CODE_GEN_H_
