//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_CALC_H_
#define KCC_SRC_CALC_H_

#include <cassert>
#include <cstdint>

#include "ast.h"
#include "error.h"
#include "visitor.h"

namespace kcc {

// 整数常量表达式是仅由下列内容组成的表达式
// 赋值、自增、自减、函数调用或逗号以外的运算符，除了转型运算符只能转型算术类型为整数类型
// 整数常量
// 枚举常量
// 字符常量
// 浮点常量，但仅若立即以它们为转型到整数类型的运算数才行
// 运算数非 VLA 的 (C99 起)sizeof 运算符
// _Alignof 运算符(C11 起)
template <typename T>
class CalcExpr : public Visitor {
 public:
  T Calc(Expr* expr);

 private:
  virtual void Visit(const UnaryOpExpr& node) override;
  virtual void Visit(const TypeCastExpr& node) override;
  virtual void Visit(const BinaryOpExpr& node) override;
  virtual void Visit(const ConditionOpExpr& node) override;
  virtual void Visit(const ConstantExpr& node) override;
  virtual void Visit(const EnumeratorExpr& node) override;
  virtual void Visit(const StmtExpr& node) override;

  virtual void Visit(const StringLiteralExpr& node) override;
  virtual void Visit(const FuncCallExpr& node) override;
  virtual void Visit(const IdentifierExpr& node) override;
  virtual void Visit(const ObjectExpr& node) override;

  virtual void Visit(const LabelStmt& node) override;
  virtual void Visit(const CaseStmt& node) override;
  virtual void Visit(const DefaultStmt& node) override;
  virtual void Visit(const CompoundStmt& node) override;
  virtual void Visit(const ExprStmt& node) override;
  virtual void Visit(const IfStmt& node) override;
  virtual void Visit(const SwitchStmt& node) override;
  virtual void Visit(const WhileStmt& node) override;
  virtual void Visit(const DoWhileStmt& node) override;
  virtual void Visit(const ForStmt& node) override;
  virtual void Visit(const GotoStmt& node) override;
  virtual void Visit(const ContinueStmt& node) override;
  virtual void Visit(const BreakStmt& node) override;
  virtual void Visit(const ReturnStmt& node) override;

  virtual void Visit(const TranslationUnit& node) override;
  virtual void Visit(const Declaration& node) override;
  virtual void Visit(const FuncDef& node) override;

 private:
  T val_;
};

template <typename T>
T CalcExpr<T>::Calc(Expr* expr) {
  expr->Accept(*this);
  return val_;
}

template <typename T>
void CalcExpr<T>::Visit(const UnaryOpExpr& node) {
  switch (node.op_) {
    case Tag::kPlus:
      val_ = CalcExpr<T>{}.Calc(node.expr_);
      break;
    case Tag::kMinus:
      val_ = -CalcExpr<T>{}.Calc(node.expr_);
      break;
    case Tag::kTilde:
      val_ = ~CalcExpr<std::uint64_t>{}.Calc(node.expr_);
      break;
    case Tag::kExclaim:
      val_ = !CalcExpr<T>{}.Calc(node.expr_);
      break;
    case Tag::kAmp:
      // TODO 地址常量表达式
      Error(node.expr_, "not support '&' in constant expression");
    default:
      Error(node.expr_, "expect constant expression");
  }
}

// 只能转型算术类型为整数类型
template <typename T>
void CalcExpr<T>::Visit(const TypeCastExpr& node) {
  if (node.expr_->GetType()->IsArithmeticTy() && node.to_->IsIntegerTy()) {
    val_ = CalcExpr<T>{}.Calc(node.expr_);
  } else {
    Error(node.expr_, "expect constant expression");
  }
}

template <typename T>
void CalcExpr<T>::Visit(const BinaryOpExpr& node) {
#define L CalcExpr<T>{}.Calc(node.lhs_)
#define R CalcExpr<T>{}.Calc(node.rhs_)
#define I_L CalcExpr<std::uint64_t>{}.Calc(node.lhs_)
#define I_R CalcExpr<std::uint64_t>{}.Calc(node.rhs_)

  switch (node.op_) {
    case Tag::kPlus:
      val_ = L + R;
      break;
    case Tag::kMinus:
      val_ = L - R;
      break;
    case Tag::kStar:
      val_ = L * R;
      break;
    case Tag::kSlash: {
      auto lhs{L};
      auto rhs{R};
      if (rhs == 0) {
        Error(node.rhs_, "division by zero");
      }
      val_ = lhs / rhs;
      break;
    }
    case Tag::kPercent: {
      auto lhs{I_L};
      auto rhs{I_R};
      if (rhs == 0) {
        Error(node.rhs_, "division by zero");
      }
      val_ = lhs / rhs;
      break;
    }
    case Tag::kAmp:
      val_ = I_L & I_R;
      break;
    case Tag::kPipe:
      val_ = I_L | I_R;
      break;
    case Tag::kCaret:
      val_ = I_L ^ I_R;
      break;
    case Tag::kLessLess:
      val_ = I_L << I_R;
      break;
    case Tag::kGreaterGreater:
      val_ = I_L >> I_R;
      break;
    case Tag::kAmpAmp:
      val_ = L && R;
      break;
    case Tag::kPipePipe:
      val_ = L || R;
      break;
    case Tag::kEqualEqual:
      val_ = L == R;
      break;
    case Tag::kExclaimEqual:
      val_ = L != R;
      break;
    case Tag::kLess:
      val_ = L < R;
      break;
    case Tag::kGreater:
      val_ = L > R;
      break;
    case Tag::kLessEqual:
      val_ = L <= R;
      break;
    case Tag::kGreaterEqual:
      val_ = L >= R;
      break;
    case Tag::kPeriod:
      // TODO 地址常量表达式
      Error(node.lhs_, "not support '.' in constant expression");
    default:
      Error(node.lhs_, "expect constant expression");
  }

#undef L
#undef R
#undef I_L
#undef I_R
}

template <typename T>
void CalcExpr<T>::Visit(const ConditionOpExpr& node) {
  auto cond_type{node.GetType()};
  bool flag{};

  if (cond_type->IsIntegerTy()) {
    flag = CalcExpr<std::int32_t>{}.Calc(node.cond_);
  } else if (cond_type->IsFloatPointTy()) {
    flag = CalcExpr<double>{}.Calc(node.cond_);
  } else if (cond_type->IsPointerTy()) {
    // TODO 地址常量表达式
    Error(node.cond_,
          "not support pointer in consition op constant expression");
  } else {
    assert(false);
  }

  if (flag) {
    val_ = CalcExpr<T>{}.Calc(node.lhs_);
  } else {
    val_ = CalcExpr<T>{}.Calc(node.rhs_);
  }
}

template <typename T>
void CalcExpr<T>::Visit(const ConstantExpr& node) {
  if (node.GetType()->IsIntegerTy()) {
    val_ = static_cast<T>(node.GetIntegerVal());
  } else if (node.GetType()->IsFloatPointTy()) {
    val_ = static_cast<T>(node.GetFloatPointVal());
  } else {
    assert(false);
  }
}

template <typename T>
void CalcExpr<T>::Visit(const EnumeratorExpr& node) {
  val_ = static_cast<T>(node.GetVal());
}

template <typename T>
void CalcExpr<T>::Visit(const StmtExpr& node) {
  if (!node.GetType()->IsVoidTy()) {
    auto last{node.block_->GetStmts().back()};
    assert(last->Kind() == AstNodeType::kExprStmt);
    val_ = CalcExpr<T>{}.Calc(dynamic_cast<ExprStmt*>(last)->GetExpr());
  } else {
    Error(node.GetLoc(), "expect constant expression");
  }
}

template <typename T>
void CalcExpr<T>::Visit(const StringLiteralExpr& node) {
  Error(node.GetLoc(), "expect constant expression");
}

template <typename T>
void CalcExpr<T>::Visit(const FuncCallExpr& node) {
  Error(node.GetLoc(), "expect constant expression");
}

template <typename T>
void CalcExpr<T>::Visit(const IdentifierExpr& node) {
  Error(node.GetLoc(), "expect constant expression");
}

template <typename T>
void CalcExpr<T>::Visit(const ObjectExpr& node) {
  Error(node.GetLoc(), "expect constant expression");
}

template <typename T>
void CalcExpr<T>::Visit(const LabelStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const CaseStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const DefaultStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const CompoundStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const ExprStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const IfStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const SwitchStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const WhileStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const DoWhileStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const ForStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const GotoStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const ContinueStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const BreakStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const ReturnStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const TranslationUnit&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const Declaration&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const FuncDef&) {
  assert(false);
}

}  // namespace kcc

#endif  // KCC_SRC_CALC_H_
