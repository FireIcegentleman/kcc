//
// Created by kaiser on 2019/11/1.
//

#ifndef KCC_SRC_JSON_GEN_H_
#define KCC_SRC_JSON_GEN_H_

#include <QJsonObject>
#include <memory>
#include <string>

#include "ast.h"
#include "visitor.h"

namespace kcc {

class JsonGen : public Visitor {
 public:
  void GenJson(const std::shared_ptr<TranslationUnit>& root,
               const std::string& file_name);

 private:
  virtual void Visit(const UnaryOpExpr& node) override;
  virtual void Visit(const BinaryOpExpr& node) override;
  virtual void Visit(const ConditionOpExpr& node) override;
  virtual void Visit(const TypeCastExpr& node) override;
  virtual void Visit(const Constant& node) override;
  virtual void Visit(const Enumerator& node) override;

  virtual void Visit(const CompoundStmt& node) override;
  virtual void Visit(const IfStmt& node) override;
  virtual void Visit(const ReturnStmt& node) override;
  virtual void Visit(const LabelStmt& node) override;
  virtual void Visit(const FuncCallExpr& node) override;
  virtual void Visit(const Identifier& node) override;
  virtual void Visit(const Object& node) override;
  virtual void Visit(const TranslationUnit& node) override;
  virtual void Visit(const JumpStmt& node) override;
  virtual void Visit(const Declaration& node) override;
  virtual void Visit(const FuncDef& node) override;
  virtual void Visit(const ExprStmt& node) override;
  virtual void Visit(const WhileStmt& node) override;
  virtual void Visit(const DoWhileStmt& node) override;
  virtual void Visit(const ForStmt& node) override;
  virtual void Visit(const CaseStmt& node) override;
  virtual void Visit(const DefaultStmt& node) override;
  virtual void Visit(const SwitchStmt& node) override;

  QJsonObject result_;
};

}  // namespace kcc

#endif  // KCC_SRC_JSON_GEN_H_
