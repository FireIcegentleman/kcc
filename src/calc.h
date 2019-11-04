//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_CALC_H_
#define KCC_SRC_CALC_H_

#include <memory>

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
  T Calc(std::shared_ptr<Expr> expr);

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

 private:
  T val_;
};

template <typename T>
T CalcExpr<T>::Calc(std::shared_ptr<Expr> expr) {
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
      // TODO 地址常量表达式
    default:
      Error(node.expr_->GetToken(), "expect constant expression");
  }
}

/*
 * + - * / % & | ^ << >>
 * && ||
 * == != < > <= >= .
 */
template <typename T>
void CalcExpr<T>::Visit(const BinaryOpExpr& node) {
#define L CalcExpr<T>{}.Calc(node.lhs_)
#define R CalcExpr<T>{}.Calc(node.rhs_)
#define I_L CalcExpr<std::int64_t>{}.Calc(node.lhs_)
#define I_R CalcExpr<std::int64_t>{}.Calc(node.rhs_)

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
        Error(node.rhs_->GetToken(), "division by zero");
      }
      val_ = lhs / rhs;
      break;
    }
    case Tag::kPercent: {
      if (!node.lhs_->GetType()->IsIntegerTy() ||
          !node.rhs_->GetType()->IsIntegerTy()) {
        Error(node.lhs_->GetToken(), "Need integer type");
      }
      auto lhs{I_L};
      auto rhs{I_R};
      if (rhs == 0) {
        Error(node.rhs_->GetToken(), "division by zero");
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
      assert(false);
      break;
    default:
      Error(node.lhs_->GetToken(), "expect constant expression");
  }
}

template <typename T>
void CalcExpr<T>::Visit(const ConditionOpExpr& node) {
  auto cond_type{node.GetType()};
  bool flag;

  if (cond_type->IsIntegerTy()) {
    flag = CalcExpr<std::int32_t>{}.Calc(node.cond_);
  } else if (cond_type->IsFloatPointTy()) {
    flag = CalcExpr<double>{}.Calc(node.cond_);
  } else if (cond_type->IsPointerTy()) {
    // TODO
    assert(false);
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
void CalcExpr<T>::Visit(const TypeCastExpr& node) {
  if (node.expr_->GetType()->IsArithmeticTy() && node.to_->IsIntegerTy()) {
    val_ = CalcExpr<T>{}.Calc(node.expr_);
  } else {
    Error(node.expr_->GetToken(), "expect constant expression");
  }
}

template <typename T>
void CalcExpr<T>::Visit(const Constant& node) {
  if (node.GetType()->IsIntegerTy()) {
    val_ = static_cast<T>(node.GetIntVal());
  } else if (node.GetType()->IsFloatTy()) {
    val_ = static_cast<T>(node.GetFloatVal());
  } else {
    assert(false);
  }
}

template <typename T>
void CalcExpr<T>::Visit(const Enumerator& node) {
  val_ = static_cast<T>(node.GetVal());
}

template <typename T>
void CalcExpr<T>::Visit(const CompoundStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const IfStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const ReturnStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const LabelStmt&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const FuncCallExpr&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const Identifier&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const Object&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const TranslationUnit&) {
  assert(false);
}

template <typename T>
void CalcExpr<T>::Visit(const JumpStmt&) {
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
