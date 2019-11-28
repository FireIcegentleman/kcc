//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_CODE_GEN_H_
#define KCC_SRC_CODE_GEN_H_

#include <cstdint>
#include <stack>
#include <string>
#include <unordered_map>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/ValueHandle.h>

#include "ast.h"
#include "visitor.h"

namespace kcc {

class CodeGen : public Visitor {
 public:
  CodeGen();
  void GenCode(const TranslationUnit *root);

 private:
  struct BreakContinue {
   public:
    BreakContinue(llvm::BasicBlock *break_block,
                  llvm::BasicBlock *continue_block);

    llvm::BasicBlock *break_block;
    llvm::BasicBlock *continue_block;
  };

  static llvm::BasicBlock *CreateBasicBlock(
      const std::string &name = "", llvm::Function *parent = nullptr,
      llvm::BasicBlock *insert_before = nullptr);
  llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Type *type,
                                           std::int32_t align);
  void EmitBranchOnBoolExpr(const Expr *expr, llvm::BasicBlock *true_block,
                            llvm::BasicBlock *false_block);
  void EmitBlock(llvm::BasicBlock *bb, bool is_finished = false);
  static void EmitBranch(llvm::BasicBlock *target);
  void EmitStmt(const Stmt *stmt);
  bool EmitSimpleStmt(const Stmt *stmt);
  bool HaveInsertPoint() const;
  void EnsureInsertPoint();
  llvm::Value *EvaluateExprAsBool(const Expr *expr);
  bool TestAndClearIgnoreResultAssign();

  void PushBlock(llvm::BasicBlock *break_stack,
                 llvm::BasicBlock *continue_block);
  void PopBlock();
  llvm::Value *GetPtr(const AstNode &node);
  static llvm::Value *Assign(llvm::Value *lhs_ptr, llvm::Value *rhs,
                             std::int32_t align);
  bool IsCheapEnoughToEvaluateUnconditionally(const Expr *expr);
  void EmitLabel(const LabelStmt& label_stmt);
  bool ContainsLabel(const Stmt* stmt, bool ignore_case= false);
  void SimplifyForwardingBlocks(llvm::BasicBlock* bb);
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

  void StartFunction(const FuncDef &node);
  void FinishFunction(const FuncDef &node);
  void EmitFunctionEpilog();
  void EmitReturnBlock();
  void EmitBranchThroughCleanup(llvm::BasicBlock *dest);

  llvm::Value *IncOrDec(const Expr &expr, bool is_inc, bool is_postfix);
  static llvm::Value *NegOp(llvm::Value *value, bool is_unsigned);
  static llvm::Value *LogicNotOp(llvm::Value *value);

  static llvm::Value *AddOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *SubOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *MulOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *DivOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *ModOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *OrOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *AndOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *XorOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *ShlOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *ShrOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *LessEqualOp(llvm::Value *lhs, llvm::Value *rhs,
                                  bool is_unsigned);
  static llvm::Value *LessOp(llvm::Value *lhs, llvm::Value *rhs,
                             bool is_unsigned);
  static llvm::Value *GreaterEqualOp(llvm::Value *lhs, llvm::Value *rhs,
                                     bool is_unsigned);
  static llvm::Value *GreaterOp(llvm::Value *lhs, llvm::Value *rhs,
                                bool is_unsigned);
  static llvm::Value *EqualOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *NotEqualOp(llvm::Value *lhs, llvm::Value *rhs);
  llvm::Value *LogicOrOp(const BinaryOpExpr &node);
  llvm::Value *LogicAndOp(const BinaryOpExpr &node);
  llvm::Value *AssignOp(const BinaryOpExpr &node);

  bool MayCallBuiltinFunc(const FuncCallExpr &node);
  llvm::Value *VaArg(llvm::Value *ptr, llvm::Type *type);

  llvm::BasicBlock *GetBasicBlockForLabel(const LabelStmt *label);

  void DealLocaleDecl(const Declaration &node);
  void InitLocalAggregate(const Declaration &node);

  llvm::Value *result_{};

  std::stack<BreakContinue> break_continue_stack_;
  std::unordered_map<const LabelStmt *, llvm::BasicBlock *> labels_;
  llvm::SwitchInst *switch_inst_{};

  llvm::Function *func_{};
  llvm::Function *va_start_{};
  llvm::Function *va_end_{};
  llvm::Function *va_copy_{};

  // 是否加载等号左边的值
  bool ignore_result_assign_{false};

  llvm::AssertingVH<llvm::Instruction> alloc_insert_point_;
  llvm::BasicBlock *return_block_{};
  llvm::Value *return_value_{};

  // temp
  std::int32_t align_{};
  bool load_{false};
};

}  // namespace kcc

#endif  // KCC_SRC_CODE_GEN_H_
