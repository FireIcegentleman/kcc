//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_CODE_GEN_H_
#define KCC_SRC_CODE_GEN_H_

#include <llvm/IR/Value.h>

#include <memory>

#include "ast.h"
#include "visitor.h"

namespace kcc {

class CodeGen : public Visitor {
 public:
  CodeGen();

  void GenCode(const std::shared_ptr<TranslationUnit> &root);

 private:
  virtual void Visit(const UnaryOpExpr &node) override;
  virtual void Visit(const BinaryOpExpr &node) override;
  virtual void Visit(const ConditionOpExpr &node) override;
  virtual void Visit(const TypeCastExpr &node) override;
  virtual void Visit(const Constant &node) override;
  virtual void Visit(const Enumerator &node) override;
  virtual void Visit(const CompoundStmt &node) override;
  virtual void Visit(const IfStmt &node) override;
  virtual void Visit(const ReturnStmt &node) override;
  virtual void Visit(const LabelStmt &node) override;
  virtual void Visit(const FuncCallExpr &node) override;
  virtual void Visit(const Identifier &node) override;
  virtual void Visit(const Object &node) override;
  virtual void Visit(const TranslationUnit &node) override;
  virtual void Visit(const Declaration &node) override;
  virtual void Visit(const FuncDef &node) override;
  virtual void Visit(const ExprStmt &node) override;
  virtual void Visit(const WhileStmt &node) override;
  virtual void Visit(const DoWhileStmt &node) override;
  virtual void Visit(const ForStmt &node) override;
  virtual void Visit(const CaseStmt &node) override;
  virtual void Visit(const DefaultStmt &node) override;
  virtual void Visit(const SwitchStmt &node) override;
  virtual void Visit(const BreakStmt &node) override;
  virtual void Visit(const ContinueStmt &node) override;
  virtual void Visit(const GotoStmt &node) override;

 private:
  llvm::Value *result_{};
};

}  // namespace kcc

#endif  // KCC_SRC_CODE_GEN_H_
