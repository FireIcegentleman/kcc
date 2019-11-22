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
  llvm::Value *LogicOrOp(const BinaryOpExpr &node);
  llvm::Value *LogicAndOp(const BinaryOpExpr &node);
  llvm::Value *AssignOp(const BinaryOpExpr &node);
  llvm::Value *GetPtr(const AstNode &node);
  llvm::Value *Assign(llvm::Value *lhs_ptr, llvm::Value *rhs,
                      std::int32_t align);
  llvm::Value *NegOp(llvm::Value *value, bool is_unsigned);
  llvm::Value *LogicNotOp(llvm::Value *value);
  llvm::Value *IncOrDec(const Expr &expr, bool is_inc, bool is_postfix);
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

  llvm::BasicBlock *CreateBasicBlock(const std::string &name = "",
                                     llvm::Function *parent = nullptr,
                                     llvm::BasicBlock *insert_before = nullptr);
  void EmitBranchOnBoolExpr(const Expr *expr, llvm::BasicBlock *true_block,
                            llvm::BasicBlock *false_block);
  void EmitBlock(llvm::BasicBlock *bb, bool is_finished = false);
  void EmitBranch(llvm::BasicBlock *target);

  void DealGlobalDecl(const Declaration &node);
  void DealLocaleDecl(const Declaration &node);
  void InitLocalAggregate(const Declaration &node);

  void PushBlock(llvm::BasicBlock *continue_block,
                 llvm::BasicBlock *break_stack);
  void PopBlock();

  bool MayCallBuiltinFunc(const FuncCallExpr &node);
  llvm::Value *VaArg(llvm::Value *ptr, llvm::Type *type);

  llvm::Value *result_{};
  std::int32_t align_{};

  llvm::Function *va_start_{};
  llvm::Function *va_end_{};
  llvm::Function *va_copy_{};

  std::map<llvm::Constant *, llvm::Constant *> strings_;

  bool load_{false};

  llvm::Function *curr_func_{};
  struct BreakContinue {
    BreakContinue(llvm::BasicBlock *break_block,
                  llvm::BasicBlock *continue_block);
    llvm::BasicBlock *break_block;
    llvm::BasicBlock *continue_block;
  };
  std::vector<BreakContinue> break_continue_stack_;
  llvm::Value *EvaluateExprAsBool(const Expr *expr);
  llvm::SwitchInst *switch_inst_{};
  std::map<const LabelStmt *, llvm::BasicBlock *> label_map_;
  llvm::BasicBlock *GetBasicBlockForLabel(const LabelStmt *label);
  bool TestAndClearIgnoreResultAssign();
  bool ignore_result_assign_{false};
  void EmitStmt(const Stmt *stmt);
  bool EmitSimpleStmt(const Stmt *stmt);
  bool HaveInsertPoint() const;
  void EnsureInsertPoint();
};

}  // namespace kcc

#endif  // KCC_SRC_CODE_GEN_H_
