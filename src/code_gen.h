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
#include "type.h"
#include "visitor.h"

namespace kcc {

// TODO  & -> *
class CodeGen : public Visitor {
 public:
  CodeGen();
  void GenCode(const TranslationUnit *root);

 private:
  class LValue {
   public:
    static LValue MakeAddr(llvm::Value *value);
    llvm::Value *GetAddr() const;

   private:
    llvm::Value *value_{};
  };

  class RValue {
   public:
    static RValue Get(llvm::Value *value);
    static RValue GetAggregate(llvm::Value *value);

    bool IsScalar() const;
    bool IsAggregate() const;

    llvm::Value *GetScalarVal() const;
    llvm::Value *GetAggregateAddr() const;

   private:
    enum { kScalar, kAggregate } flavor_;
    llvm::Value *value_{};
  };

  struct BreakContinue {
    BreakContinue(llvm::BasicBlock *break_block,
                  llvm::BasicBlock *continue_block);

    llvm::BasicBlock *break_block;
    llvm::BasicBlock *continue_block;
  };

  static llvm::BasicBlock *CreateBasicBlock(
      const std::string &name = "", llvm::Function *parent = nullptr,
      llvm::BasicBlock *insert_before = nullptr);
  llvm::AllocaInst *CreateTempAlloca(QualType type);
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
  static bool ContainsLabel(const Stmt *stmt, bool ignore_case = false);
  static void SimplifyForwardingBlocks(llvm::BasicBlock *bb);
  void EmitBranchThroughCleanup(llvm::BasicBlock *dest);
  static bool IsCheapEnoughToEvaluateUnconditionally(const Expr *expr);
  void PushBlock(llvm::BasicBlock *break_stack,
                 llvm::BasicBlock *continue_block);
  void PopBlock();

  LValue EmitLValue(const Expr &expr);
  LValue EmitUnaryLValue(const UnaryOpExpr &unary);
  LValue EmitBinaryLValue(const BinaryOpExpr &binary);
  static LValue EmitObjectLValue(const ObjectExpr &obj);
  LValue EmitIdentifierLValue(const IdentifierExpr &ident);

  llvm::Value *EmitLoadOfLValue(const Expr *expr);
  static RValue EmitLoadOfLValue(LValue l_value, QualType type);
  static void EmitStoreThroughLValue(RValue src, LValue dst, QualType type);

  static llvm::Value *EmitLoadOfScalar(llvm::Value *addr, QualType type);
  static void EmitStoreOfScalar(llvm::Value *value, llvm::Value *addr,
                                QualType type);

  void EmitAggLoadOfLValue(const Expr *expr);
  void EmitFinalDestCopy(const Expr *expr, LValue src, bool ignore = false);
  static void EmitAggregateCopy(llvm::Value *dest_ptr, llvm::Value *src_ptr,
                                QualType type);

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
  void EmitFunctionEpilog(const FuncDef &node);
  void EmitReturnBlock();

  void EmitLabel(const LabelStmt &label_stmt);
  llvm::Value *VisitPrePostIncDec(const UnaryOpExpr &unary, bool is_inc,
                                  bool is_postfix);
  llvm::Value *VisitUnaryPreDec(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryPreInc(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryPostDec(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryPostInc(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryAddr(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryDeref(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryPlus(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryMinus(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryNot(const UnaryOpExpr &unary);
  llvm::Value *VisitUnaryLogicNot(const UnaryOpExpr &unary);
  llvm::Value *VisitBinaryMemRef(const BinaryOpExpr &binary);

  void VisitAggBinaryMemRef(const BinaryOpExpr &binary);
  void VisitAggUnaryDeref(const UnaryOpExpr &unary);
  void VisitAggBinaryAssign(const BinaryOpExpr &node);
  void VisitConditionOpExpr(const ConditionOpExpr &node);

  static llvm::Value *NegOp(llvm::Value *value, bool is_unsigned);
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
  // 处理语句表达式时, 是否获取最后一个语句的值
  bool get_last_{false};
  // 最后一个语句的值
  RValue last_value_;
  llvm::Value *dest_ptr_{};
  bool ignore_result_{false};
  llvm::AssertingVH<llvm::Instruction> alloc_insert_point_;
  llvm::BasicBlock *return_block_{};
  llvm::Value *return_value_{};
};

}  // namespace kcc

#endif  // KCC_SRC_CODE_GEN_H_
