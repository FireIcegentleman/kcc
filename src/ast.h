//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_AST_H_
#define KCC_SRC_AST_H_

#include <memory>
#include <utility>
#include <vector>

#include "token.h"

namespace kcc {

class Visitor;

class AstNode {
 public:
  AstNode() = default;
  virtual ~AstNode() = default;
  virtual void Accept(Visitor& visitor) const;
  void SetLoc(const SourceLocation& loc) { loc_ = loc; }
  SourceLocation GetLoc() const { return loc_; }

 private:
  SourceLocation loc_;
};

class Stmt : public AstNode {};

class Expr : public AstNode {};

class BinaryOpExpr : public Expr {
 public:
  BinaryOpExpr(Tag op, std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs)
      : op_(op), lhs_(lhs), rhs_(rhs) {}
  virtual void Accept(Visitor& visitor) const;

 private:
  Tag op_;
  std::shared_ptr<Expr> lhs_;
  std::shared_ptr<Expr> rhs_;
};

class UnaryOpExpr : public Expr {
 public:
  UnaryOpExpr(Tag op, std::shared_ptr<Expr> expr) : op_(op), expr_(expr) {}
  virtual void Accept(Visitor& visitor) const;

 private:
  Tag op_;
  std::shared_ptr<Expr> expr_;
};

class ConditionOpExpr : public Expr {
 public:
  ConditionOpExpr(std::shared_ptr<Expr> cond, std::shared_ptr<Expr> true_expr,
                  std::shared_ptr<Expr> false_expr)
      : cond_(cond), true_expr_(true_expr), false_expr_(false_expr) {}
  virtual void Accept(Visitor& visitor) const;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Expr> true_expr_;
  std::shared_ptr<Expr> false_expr_;
};

class TranslationUnit : public AstNode {
 public:
  virtual void Accept(Visitor& visitor) const override;
  void AddStmt(std::shared_ptr<Stmt> stmt) { stmts_.push_back(stmt); }

 private:
  std::vector<std::shared_ptr<Stmt>> stmts_;
};

}  // namespace kcc

#endif  // KCC_SRC_AST_H_
