//
// Created by kaiser on 2019/10/31.
//

#include "ast.h"

#include <algorithm>

#include "calc.h"
#include "error.h"
#include "memory_pool.h"
#include "visitor.h"

namespace kcc {

#define ACCEPT(ClassName) \
  void ClassName::Accept(Visitor& visitor) const { visitor.Visit(*this); }

ACCEPT(UnaryOpExpr)
ACCEPT(TypeCastExpr)
ACCEPT(BinaryOpExpr)
ACCEPT(ConditionOpExpr)
ACCEPT(FuncCallExpr)
ACCEPT(ConstantExpr)
ACCEPT(StringLiteralExpr)
ACCEPT(IdentifierExpr)
ACCEPT(EnumeratorExpr)
ACCEPT(ObjectExpr)
ACCEPT(StmtExpr)

ACCEPT(LabelStmt)
ACCEPT(CaseStmt)
ACCEPT(DefaultStmt)
ACCEPT(CompoundStmt)
ACCEPT(ExprStmt)
ACCEPT(IfStmt)
ACCEPT(SwitchStmt)
ACCEPT(WhileStmt)
ACCEPT(DoWhileStmt)
ACCEPT(ForStmt)
ACCEPT(GotoStmt)
ACCEPT(ContinueStmt)
ACCEPT(BreakStmt)
ACCEPT(ReturnStmt)

ACCEPT(TranslationUnit)
ACCEPT(Declaration)
ACCEPT(FuncDef)

/*
 * AstNodeTypes
 */
QString AstNodeTypes::ToString(AstNodeTypes::Type type) {
  return QMetaEnum::fromType<Type>().valueToKey(type) + 1;
}

/*
 * AstNode
 */
QString AstNode::KindStr() const { return AstNodeTypes::ToString(Kind()); }

Location AstNode::GetLoc() const { return loc_; }

void AstNode::SetLoc(const Location& loc) { loc_ = loc; }

/*
 * Expr
 */
QualType Expr::GetQualType() const { return type_; }

Type* Expr::GetType() { return type_.GetType(); }

const Type* Expr::GetType() const { return type_.GetType(); }

bool Expr::IsConst() const { return type_.IsConst(); }

bool Expr::IsRestrict() const { return type_.IsRestrict(); }

void Expr::EnsureCompatible(QualType lhs, QualType rhs) const {
  if (!lhs->Compatible(rhs.GetType())) {
    Error(loc_, "incompatible types: '{}' vs '{}'", lhs->ToString(),
          rhs->ToString());
  }
}

void Expr::EnsureCompatibleOrVoidPtr(QualType lhs, QualType rhs) const {
  if ((lhs->IsPointerTy() && rhs->IsPointerTy()) &&
      (lhs->PointerGetElementType()->IsVoidTy() ||
       rhs->PointerGetElementType()->IsVoidTy())) {
    return;
  } else {
    return EnsureCompatible(lhs, rhs);
  }
}

Expr* Expr::MayCast(Expr* expr) {
  auto type{Type::MayCast(expr->GetQualType())};

  if (type != expr->GetQualType()) {
    return MakeNode<TypeCastExpr>(expr->GetLoc(), expr, type);
  } else {
    return expr;
  }
}

Expr* Expr::MayCastTo(Expr* expr, QualType to) {
  expr = MayCast(expr);

  if (!expr->GetQualType()->Equal(to.GetType())) {
    return MakeNode<TypeCastExpr>(expr->GetLoc(), expr, to);
  } else {
    return expr;
  }
}

Type* Expr::Convert(Expr*& lhs, Expr*& rhs) {
  auto type{QualType{ArithmeticType::MaxType(lhs->GetType(), rhs->GetType())}};

  lhs = Expr::MayCastTo(lhs, type);
  rhs = Expr::MayCastTo(rhs, type);

  return type.GetType();
}

Expr::Expr(QualType type) : type_{type} {}

/*
 * UnaryOpExpr
 */
UnaryOpExpr* UnaryOpExpr::Get(Tag tag, Expr* expr) {
  return new (UnaryOpExprPool.Allocate()) UnaryOpExpr{tag, expr};
}

AstNodeType UnaryOpExpr::Kind() const { return AstNodeType::kUnaryOpExpr; }

void UnaryOpExpr::Check() {
  switch (op_) {
    case Tag::kPlusPlus:
    case Tag::kMinusMinus:
    case Tag::kPostfixPlusPlus:
    case Tag::kPostfixMinusMinus:
      IncDecOpCheck();
      break;
    case Tag::kPlus:
    case Tag::kMinus:
      UnaryAddSubOpCheck();
      break;
    case Tag::kTilde:
      NotOpCheck();
      break;
    case Tag::kExclaim:
      LogicNotOpCheck();
      break;
    case Tag::kStar:
      DerefOpCheck();
      break;
    case Tag::kAmp:
      AddrOpCheck();
      break;
    default:
      assert(false);
  }
}

bool UnaryOpExpr::IsLValue() const { return op_ == Tag::kStar; }

UnaryOpExpr::UnaryOpExpr(Tag tag, Expr* expr) : op_(tag) {
  if (op_ == Tag::kAmp) {
    expr_ = expr;
  } else {
    expr_ = Expr::MayCast(expr);
  }
}

void UnaryOpExpr::IncDecOpCheck() {
  auto expr_type{expr_->GetQualType()};

  if (expr_->IsConst()) {
    Error(loc_, "'++' / '--': left operand of '=' is const qualified: {}",
          expr_type->ToString());
  } else if (!expr_->IsLValue()) {
    Error(loc_, "'++' / '--': lvalue expression expected: {}",
          expr_type->ToString());
  }

  if (expr_type->IsRealTy() || expr_type->IsPointerTy()) {
    type_ = expr_type;
  } else {
    Error(loc_, "expect operand of real type or pointer");
  }
}

void UnaryOpExpr::UnaryAddSubOpCheck() {
  if (!expr_->GetType()->IsArithmeticTy()) {
    Error(loc_, "'+' / '-': expect operand of arithmetic type: {}",
          expr_->GetType()->ToString());
  }

  auto new_type{ArithmeticType::IntegerPromote(expr_->GetQualType().GetType())};
  expr_ = Expr::MayCastTo(expr_, QualType{new_type});

  type_ = expr_->GetQualType();
}

void UnaryOpExpr::NotOpCheck() {
  if (!expr_->GetType()->IsIntegerTy()) {
    Error(loc_, "'~': expect operand of arithmetic type: {}",
          expr_->GetType()->ToString());
  }

  auto new_type{ArithmeticType::IntegerPromote(expr_->GetQualType().GetType())};
  expr_ = Expr::MayCastTo(expr_, QualType{new_type});

  type_ = expr_->GetQualType();
}

void UnaryOpExpr::LogicNotOpCheck() {
  if (!expr_->GetType()->IsScalarTy()) {
    Error(loc_, "'!': expect operand of scalar type: {}",
          expr_->GetQualType()->ToString());
  }

  type_ = ArithmeticType::Get(kInt);
}

void UnaryOpExpr::DerefOpCheck() {
  if (!expr_->GetType()->IsPointerTy()) {
    Error(loc_, "'*': expect operand of pointer type: {}",
          expr_->GetType()->ToString());
  }

  type_ = expr_->GetType()->PointerGetElementType();
}

void UnaryOpExpr::AddrOpCheck() {
  if (!expr_->GetType()->IsFunctionTy() && !expr_->IsLValue()) {
    Error(loc_, "'&': expression must be an lvalue or function: {}",
          expr_->GetQualType()->ToString());
  }

  type_ = PointerType::Get(expr_->GetQualType());
}

/*
 * TypeCastExpr
 */
TypeCastExpr* TypeCastExpr::Get(Expr* expr, QualType to) {
  return new (TypeCastExprPool.Allocate()) TypeCastExpr{expr, to};
}

AstNodeType TypeCastExpr::Kind() const { return AstNodeType::kTypeCastExpr; }

void TypeCastExpr::Check() {
  if (Type::MayCast(expr_->GetType())->Equal(to_.GetType())) {
    type_ = to_;
    return;
  }

  if (to_->IsFloatPointTy() && expr_->GetType()->IsPointerTy()) {
    Error(loc_, "cannot cast a pointer to float point: '{}' to '{}'",
          expr_->GetType()->ToString(), to_->ToString());
  } else if (to_->IsPointerTy() && expr_->GetType()->IsFloatPointTy()) {
    Error(loc_, "cannot cast a float point to pointer: '{}' to '{}'",
          expr_->GetType()->ToString(), to_->ToString());
  }

  type_ = to_;
}

bool TypeCastExpr::IsLValue() const { return false; }

TypeCastExpr::TypeCastExpr(Expr* expr, QualType to)
    : Expr(to), expr_{expr}, to_{to} {}

/*
 * BinaryOpExpr
 */
BinaryOpExpr* BinaryOpExpr::Get(Tag tag, Expr* lhs, Expr* rhs) {
  return new (BinaryOpExprPool.Allocate()) BinaryOpExpr{tag, lhs, rhs};
}

AstNodeType BinaryOpExpr::Kind() const { return AstNodeType::kBinaryOpExpr; }

void BinaryOpExpr::Check() {
  switch (op_) {
    case Tag::kEqual:
      AssignOpCheck();
      break;
    case Tag::kPlus:
      AddOpCheck();
      break;
    case Tag::kMinus:
      SubOpCheck();
      break;
    case Tag::kStar:
    case Tag::kPercent:
    case Tag::kSlash:
      MultiOpCheck();
      break;
    case Tag::kAmp:
    case Tag::kPipe:
    case Tag::kCaret:
      BitwiseOpCheck();
      break;
    case Tag::kLessLess:
    case Tag::kGreaterGreater:
      ShiftOpCheck();
      break;
    case Tag::kAmpAmp:
    case Tag::kPipePipe:
      LogicalOpCheck();
      break;
    case Tag::kExclaimEqual:
    case Tag::kEqualEqual:
      EqualityOpCheck();
      break;
    case Tag::kLess:
    case Tag::kLessEqual:
    case Tag::kGreater:
    case Tag::kGreaterEqual:
      RelationalOpCheck();
      break;
    case Tag::kPeriod:
      MemberRefOpCheck();
      break;
    case Tag::kComma:
      CommaOpCheck();
      break;
    default:
      assert(false);
  }
}

bool BinaryOpExpr::IsLValue() const {
  // 在 C++ 中赋值运算符表达式是左值表达式，逗号运算符表达式可以是左值表达式
  // 而在 C 中两者都绝不是
  switch (op_) {
    case Tag::kPeriod:
      return lhs_->IsLValue();
    default:
      return false;
  }
}

BinaryOpExpr::BinaryOpExpr(Tag tag, Expr* lhs, Expr* rhs) : op_(tag) {
  if (tag != Tag::kPeriod) {
    lhs_ = MayCast(lhs);
    rhs_ = MayCast(rhs);
  } else {
    lhs_ = lhs;
    rhs_ = rhs;
  }
}

void BinaryOpExpr::AssignOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_->IsConst()) {
    Error(lhs_, "left operand of '=' is const qualified: {}",
          lhs_type->ToString());
  } else if (!lhs_->IsLValue()) {
    Error(lhs_, "'=': lvalue expression expected: {}", lhs_type->ToString());
  }

  if ((!lhs_type->IsArithmeticTy() || !rhs_type->IsArithmeticTy()) &&
      !(lhs_type->IsBoolTy() && rhs_type->IsPointerTy())) {
    // 注意， 目前 NULL 预处理之后为 (void*)0
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
  }

  rhs_ = Expr::MayCastTo(rhs_, lhs_type);
  type_ = lhs_type;
}

void BinaryOpExpr::AddOpCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Expr::Convert(lhs_, rhs_);
  } else if (lhs_type->IsIntegerTy() && rhs_type->IsPointerTy()) {
    lhs_ = Expr::MayCastTo(lhs_, ArithmeticType::Get(kLong | kUnsigned));
    type_ = rhs_type;
    // 保持 lhs 是指针
    std::swap(lhs_, rhs_);
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    rhs_ = Expr::MayCastTo(rhs_, ArithmeticType::Get(kLong | kUnsigned));
    type_ = lhs_type;
  } else {
    Error(loc_,
          "'+' the operand should be integer or pointer type '{}' vs '{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }
}

void BinaryOpExpr::SubOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Expr::Convert(lhs_, rhs_);
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    type_ = lhs_type;
  } else if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    // ptrdiff_t
    type_ = QualType{ArithmeticType::Get(kLong)};
  } else {
    Error(loc_,
          "'-' the operand should be integer or pointer type '{}' vs '{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }
}

void BinaryOpExpr::MultiOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (op_ == Tag::kPercent) {
    if (!lhs_type->IsIntegerTy() || !rhs_type->IsIntegerTy()) {
      Error(loc_, "'%': the operand should be integer type '{}' vs '{}'",
            lhs_type->ToString(), rhs_type->ToString());
    }
  } else {
    if (!lhs_type->IsArithmeticTy() || !rhs_type->IsArithmeticTy()) {
      Error(loc_,
            "'*' or '/': the operand should be arithmetic type '{}' vs '{}'",
            lhs_type->ToString(), rhs_type->ToString());
    }
  }

  type_ = Expr::Convert(lhs_, rhs_);
}

void BinaryOpExpr::BitwiseOpCheck() {
  if (!lhs_->GetQualType()->IsIntegerTy() ||
      !rhs_->GetQualType()->IsIntegerTy()) {
    Error(loc_, "'{}': the operand should be Integer type '{}' vs '{}'",
          TokenTag::ToString(op_), lhs_->GetType()->ToString(),
          rhs_->GetType()->ToString());
  }

  type_ = Expr::Convert(lhs_, rhs_);
}

void BinaryOpExpr::ShiftOpCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (!lhs_type->IsIntegerTy() || !rhs_type->IsIntegerTy()) {
    Error(loc_,
          "'<<' or '>>': the operand should be arithmetic type '{}' vs '{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }

  lhs_ = Expr::MayCastTo(lhs_, ArithmeticType::IntegerPromote(lhs_type));
  rhs_ = Expr::MayCastTo(rhs_, ArithmeticType::IntegerPromote(rhs_type));

  // LLVM 指令要求类型一致
  // TODO 更正确的形式
  rhs_ = Expr::MayCastTo(rhs_, lhs_->GetType());

  type_ = lhs_->GetQualType();
}

void BinaryOpExpr::LogicalOpCheck() {
  std::uint32_t is_unsigned{};

  if (!lhs_->GetQualType()->IsScalarTy() ||
      !rhs_->GetQualType()->IsScalarTy()) {
    Error(loc_, "'&&' or '||': the operand should be scalar type '{}' vs '{}'",
          lhs_->GetQualType()->ToString(), rhs_->GetQualType()->ToString());
  }

  is_unsigned = (lhs_->GetType()->IsUnsigned() || rhs_->GetType()->IsUnsigned())
                    ? kUnsigned
                    : 0;

  type_ = ArithmeticType::Get(kInt | is_unsigned);
}

void BinaryOpExpr::EqualityOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};
  std::uint32_t is_unsigned{};

  if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
  } else if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    Expr::Convert(lhs_, rhs_);
    is_unsigned =
        (lhs_->GetType()->IsUnsigned() || rhs_->GetType()->IsUnsigned())
            ? kUnsigned
            : 0;
  } else {
    Error(loc_, "'==' or '!=': the operand should be integer type '{}' vs '{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }

  type_ = ArithmeticType::Get(kInt | is_unsigned);
}

void BinaryOpExpr::RelationalOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};
  std::uint32_t is_unsigned{};

  if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
  } else if (lhs_type->IsRealTy() && rhs_type->IsRealTy()) {
    Expr::Convert(lhs_, rhs_);
    is_unsigned =
        (lhs_->GetType()->IsUnsigned() || rhs_->GetType()->IsUnsigned())
            ? kUnsigned
            : 0;
  } else {
    Error(loc_,
          "'<' / '>' / '<=' / '>=': the operand should be integer type '{}' vs "
          "'{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }

  type_ = ArithmeticType::Get(kInt | is_unsigned);
}

void BinaryOpExpr::MemberRefOpCheck() { type_ = rhs_->GetQualType(); }

void BinaryOpExpr::CommaOpCheck() { type_ = rhs_->GetQualType(); }

/*
 * ConditionOpExpr
 */
ConditionOpExpr* ConditionOpExpr::Get(Expr* cond, Expr* lhs, Expr* rhs) {
  return new (ConditionOpExprPool.Allocate()) ConditionOpExpr{cond, lhs, rhs};
}

AstNodeType ConditionOpExpr::Kind() const {
  return AstNodeType::kConditionOpExpr;
}

void ConditionOpExpr::Check() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(loc_, "value of type '{}' is not contextually convertible to 'bool'",
          cond_->GetType()->ToString());
  }

  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Expr::Convert(lhs_, rhs_);
  } else if (lhs_type->IsStructOrUnionTy() && rhs_type->IsStructOrUnionTy()) {
    if (!lhs_type->Equal(rhs_type)) {
      Error(loc_, "Must have the same struct or union type: '{}' vs '{}'",
            lhs_type->ToString(), rhs_type->ToString());
    }
    type_ = lhs_type;
  } else if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
    type_ = lhs_type;
  }
}

bool ConditionOpExpr::IsLValue() const {
  // 和 C++ 不同，C 的条件运算符表达式绝不是左值
  return false;
}

ConditionOpExpr::ConditionOpExpr(Expr* cond, Expr* lhs, Expr* rhs)
    : cond_(Expr::MayCast(cond)),
      lhs_(Expr::MayCast(lhs)),
      rhs_(Expr::MayCast(rhs)) {}

/*
 * FuncCallExpr
 */
FuncCallExpr* FuncCallExpr::Get(Expr* callee, std::vector<Expr*> args) {
  return new (FuncCallExprPool.Allocate()) FuncCallExpr{callee, args};
}

AstNodeType FuncCallExpr::Kind() const { return AstNodeType::kFuncCallExpr; }

void FuncCallExpr::Check() {
  if (callee_->GetType()->IsPointerTy()) {
    callee_ = MakeNode<UnaryOpExpr>(callee_->GetLoc(), Tag::kStar, callee_);
  }

  if (!callee_->GetType()->IsFunctionTy()) {
    Error(loc_, "called object is not a function or function pointer: {}",
          callee_->GetType()->ToString());
  }

  auto func_type{callee_->GetType()};
  auto args_iter{std::begin(args_)};

  for (const auto& param : callee_->GetType()->FuncGetParams()) {
    if (args_iter == std::end(args_)) {
      Error(loc_, "too few arguments for function call");
    }

    *args_iter = Expr::MayCastTo(*args_iter, param->GetType());
    ++args_iter;
  }

  if (args_iter != std::end(args_) &&
      !callee_->GetQualType()->FuncIsVarArgs()) {
    Error(loc_, "too many arguments for function call");
  }

  while (args_iter != std::end(args_)) {
    // 浮点提升
    if ((*args_iter)->GetType()->IsFloatTy()) {
      *args_iter = Expr::MayCastTo(
          *args_iter, QualType{ArithmeticType::Get(kDouble),
                               (*args_iter)->GetQualType().GetTypeQual()});
    } else if ((*args_iter)->GetQualType()->IsIntegerTy()) {
      *args_iter = Expr::MayCastTo(
          *args_iter, QualType{ArithmeticType::IntegerPromote(
                                   (*args_iter)->GetQualType().GetType()),
                               (*args_iter)->GetQualType().GetTypeQual()});
    }
    ++args_iter;
  }

  type_ = func_type->FuncGetReturnType();
}

bool FuncCallExpr::IsLValue() const { return false; }

Type* FuncCallExpr::GetFuncType() const {
  if (callee_->GetType()->IsPointerTy()) {
    return callee_->GetType()->PointerGetElementType().GetType();
  } else {
    return callee_->GetType();
  }
}

FuncCallExpr::FuncCallExpr(Expr* callee, std::vector<Expr*> args)
    : callee_{callee}, args_{std::move(args)} {}

/*
 * Constant
 */
ConstantExpr* ConstantExpr::Get(std::int32_t val) {
  return new (ConstantExprPool.Allocate()) ConstantExpr{val};
}

ConstantExpr* ConstantExpr::Get(Type* type, std::uint64_t val) {
  return new (ConstantExprPool.Allocate()) ConstantExpr{type, val};
}

ConstantExpr* ConstantExpr::Get(Type* type, long double val) {
  return new (ConstantExprPool.Allocate()) ConstantExpr{type, val};
}

AstNodeType ConstantExpr::Kind() const { return AstNodeType::kConstantExpr; }

void ConstantExpr::Check() {}

bool ConstantExpr::IsLValue() const { return false; }

long ConstantExpr::GetIntegerVal() const { return integer_val_; }

double ConstantExpr::GetFloatPointVal() const { return float_point_val_; }

ConstantExpr::ConstantExpr(std::int32_t val)
    : Expr(ArithmeticType::Get(kInt)), integer_val_(val) {}

ConstantExpr::ConstantExpr(Type* type, std::uint64_t val)
    : Expr(type), integer_val_(val) {}

ConstantExpr::ConstantExpr(Type* type, long double val)
    : Expr(type), float_point_val_(val) {}

/*
 * StringLiteral
 */
StringLiteralExpr* StringLiteralExpr::Get(const std::string& val) {
  return StringLiteralExpr::Get(ArithmeticType::Get(kChar), val);
}

StringLiteralExpr* StringLiteralExpr::Get(Type* type, const std::string& val) {
  return new (StringLiteralExprPool.Allocate()) StringLiteralExpr{type, val};
}

AstNodeType StringLiteralExpr::Kind() const {
  return AstNodeType::kStringLiteralExpr;
}

void StringLiteralExpr::Check() {}

bool StringLiteralExpr::IsLValue() const { return false; }

std::string StringLiteralExpr::GetVal() const { return val_; }

StringLiteralExpr::StringLiteralExpr(Type* type, const std::string& val)
    : Expr{ArrayType::Get(type, std::size(val) / type->GetWidth())},
      val_{val} {}

/*
 * Identifier
 */
IdentifierExpr* IdentifierExpr::Get(const std::string& name, QualType type,
                                    enum Linkage linkage, bool is_type_name) {
  return new (IdentifierExprPool.Allocate())
      IdentifierExpr{name, type, linkage, is_type_name};
}

AstNodeType IdentifierExpr::Kind() const {
  return AstNodeType::kIdentifierExpr;
}

void IdentifierExpr::Check() {}

bool IdentifierExpr::IsLValue() const { return false; }

enum Linkage IdentifierExpr::GetLinkage() const { return linkage_; }

std::string IdentifierExpr::GetName() const { return name_; }

bool IdentifierExpr::IsTypeName() const { return is_type_name_; }

bool IdentifierExpr::IsObject() const {
  return dynamic_cast<const ObjectExpr*>(this);
}

bool IdentifierExpr::IsEnumerator() const {
  return dynamic_cast<const EnumeratorExpr*>(this);
}

IdentifierExpr::IdentifierExpr(const std::string& name, QualType type,
                               enum Linkage linkage, bool is_type_name)
    : Expr{type}, name_{name}, linkage_{linkage}, is_type_name_{is_type_name} {}

/*
 * Enumerator
 */
EnumeratorExpr* EnumeratorExpr::Get(const std::string& name, std::int32_t val) {
  return new (EnumeratorExprPool.Allocate()) EnumeratorExpr{name, val};
}

AstNodeType EnumeratorExpr::Kind() const {
  return AstNodeType::kEnumeratorExpr;
}

void EnumeratorExpr::Check() {}

bool EnumeratorExpr::IsLValue() const { return false; }

std::int32_t EnumeratorExpr::GetVal() const { return val_; }

EnumeratorExpr::EnumeratorExpr(const std::string& name, std::int32_t val)
    : IdentifierExpr{name, ArithmeticType::Get(kInt), kNone, false},
      val_{val} {}

/*
 * Object
 */
ObjectExpr* ObjectExpr::Get(const std::string& name, QualType type,
                            std::uint32_t storage_class_spec,
                            enum Linkage linkage, bool anonymous) {
  return new (ObjectExprPool.Allocate())
      ObjectExpr{name, type, storage_class_spec, linkage, anonymous};
}

AstNodeType ObjectExpr::Kind() const { return AstNodeType::kObjectExpr; }

void ObjectExpr::Check() {}

bool ObjectExpr::IsLValue() const { return true; }

bool ObjectExpr::IsStatic() const { return storage_class_spec_ & kStatic; }

bool ObjectExpr::IsExtern() const { return storage_class_spec_ & kExtern; }

void ObjectExpr::SetStorageClassSpec(std::uint32_t storage_class_spec) {
  storage_class_spec_ = storage_class_spec;
}

std::uint32_t ObjectExpr::GetStorageClassSpec() { return storage_class_spec_; }

std::int32_t ObjectExpr::GetAlign() const { return align_; }

void ObjectExpr::SetAlign(std::int32_t align) { align_ = align; }

std::int32_t ObjectExpr::GetOffset() const { return offset_; }

void ObjectExpr::SetOffset(std::int32_t offset) { offset_ = offset; }

Declaration* ObjectExpr::GetDecl() { return decl_; }

void ObjectExpr::SetDecl(Declaration* decl) { decl_ = decl; }

bool ObjectExpr::HasInit() const { return decl_->HasLocalInit(); }

bool ObjectExpr::IsAnonymous() const { return anonymous_; }

bool ObjectExpr::InGlobal() const { return linkage_ != kNone; }

void ObjectExpr::SetLocalPtr(llvm::AllocaInst* local_ptr) {
  assert(local_ptr_ == nullptr && global_ptr_ == nullptr);
  local_ptr_ = local_ptr;
}

void ObjectExpr::SetGlobalPtr(llvm::GlobalVariable* global_ptr) {
  assert(local_ptr_ == nullptr && global_ptr_ == nullptr);
  global_ptr_ = global_ptr;
}

llvm::AllocaInst* ObjectExpr::GetLocalPtr() {
  assert(local_ptr_ != nullptr);
  return local_ptr_;
}

llvm::GlobalVariable* ObjectExpr::GetGlobalPtr() {
  assert(global_ptr_ != nullptr);
  return global_ptr_;
}

bool ObjectExpr::HasGlobalPtr() const { return global_ptr_ != nullptr; }

std::list<std::pair<Type*, std::int32_t>>& ObjectExpr::GetIndexs() {
  return indexs_;
}

void ObjectExpr::SetIndexs(
    const std::list<std::pair<Type*, std::int32_t>>& indexs) {
  indexs_ = indexs;
}

ObjectExpr::ObjectExpr(const std::string& name, QualType type,
                       std::uint32_t storage_class_spec, enum Linkage linkage,
                       bool anonymous)
    : IdentifierExpr{name, type, linkage, false},
      anonymous_{anonymous},
      storage_class_spec_{storage_class_spec},
      align_{type->GetAlign()} {}

/*
 * StmtExpr
 */
StmtExpr* StmtExpr::Get(CompoundStmt* block) {
  return new (StmtExprPool.Allocate()) StmtExpr{block};
}

AstNodeType StmtExpr::Kind() const { return AstNodeType::kStmtExpr; }

void StmtExpr::Check() {
  auto stmts{block_->GetStmts()};

  if (std::size(stmts) > 0 && stmts.back()->Kind() == AstNodeType::kExprStmt) {
    auto expr{dynamic_cast<ExprStmt*>(stmts.back())->GetExpr()};

    if (expr) {
      type_ = expr->GetQualType();
      return;
    }
  }

  type_ = VoidType::Get();
}

bool StmtExpr::IsLValue() const { return false; }

StmtExpr::StmtExpr(CompoundStmt* block) : block_{block} {}

/*
 * LabelStmt
 */
LabelStmt* LabelStmt::Get(IdentifierExpr* label) {
  return new (LabelStmtPool.Allocate()) LabelStmt{label};
}

AstNodeType LabelStmt::Kind() const { return AstNodeType::kLabelStmt; }

void LabelStmt::Check() {}

LabelStmt::LabelStmt(IdentifierExpr* ident) : ident_{ident} {}

/*
 * CaseStmt
 */
CaseStmt* CaseStmt::Get(std::int32_t case_value, Stmt* block) {
  return new (CaseStmtPool.Allocate()) CaseStmt{case_value, block};
}

CaseStmt* CaseStmt::Get(std::int32_t case_value, std::int32_t case_value2,
                        Stmt* block) {
  return new (CaseStmtPool.Allocate()) CaseStmt{case_value, case_value2, block};
}

AstNodeType CaseStmt::Kind() const { return AstNodeType::kCaseStmt; }

void CaseStmt::Check() {}

CaseStmt::CaseStmt(std::int32_t case_value, Stmt* block)
    : case_value_{case_value}, block_{block} {}

CaseStmt::CaseStmt(std::int32_t case_value, std::int32_t case_value2,
                   Stmt* block)
    : case_value_range_{case_value, case_value2},
      has_range_{true},
      block_{block} {}

/*
 * DefaultStmt
 */
DefaultStmt* DefaultStmt::Get(Stmt* block) {
  return new (DefaultStmtPool.Allocate()) DefaultStmt{block};
}

AstNodeType DefaultStmt::Kind() const { return AstNodeType::kDefaultStmt; }

void DefaultStmt::Check() {}

DefaultStmt::DefaultStmt(Stmt* block) : block_{block} {}

/*
 * CompoundStmt
 */
CompoundStmt* CompoundStmt::Get() {
  return new (CompoundStmtPool.Allocate()) CompoundStmt{};
}

CompoundStmt* CompoundStmt::Get(std::vector<Stmt*> stmts, Scope* scope) {
  return new (CompoundStmtPool.Allocate()) CompoundStmt{stmts, scope};
}

AstNodeType CompoundStmt::Kind() const { return AstNodeType::kCompoundStmt; }

void CompoundStmt::Check() {}

Scope* CompoundStmt::GetScope() { return scope_; }

std::vector<Stmt*> CompoundStmt::GetStmts() { return stmts_; }

void CompoundStmt::AddStmt(Stmt* stmt) {
  // 非 typedef
  if (stmt) {
    stmts_.push_back(stmt);
  }
}

CompoundStmt::CompoundStmt(std::vector<Stmt*> stmts, Scope* scope)
    : stmts_{std::move(stmts)}, scope_{scope} {}

/*
 * ExprStmt
 */
ExprStmt* ExprStmt::Get(Expr* expr) {
  return new (ExprStmtPool.Allocate()) ExprStmt{expr};
}

AstNodeType ExprStmt::Kind() const { return AstNodeType::kExprStmt; }

void ExprStmt::Check() {}

Expr* ExprStmt::GetExpr() const { return expr_; }

ExprStmt::ExprStmt(Expr* expr) : expr_{expr} {}

/*
 * IfStmt
 */
IfStmt* IfStmt::Get(Expr* cond, Stmt* then_block, Stmt* else_block) {
  return new (IfStmtPool.Allocate()) IfStmt{cond, then_block, else_block};
}

AstNodeType IfStmt::Kind() const { return AstNodeType::kIfStmt; }

void IfStmt::Check() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(cond_->GetLoc(), "expect scalar");
  }
}

IfStmt::IfStmt(Expr* cond, Stmt* then_block, Stmt* else_block)
    : cond_{cond}, then_block_{then_block}, else_block_{else_block} {}

/*
 * SwitchStmt
 */
SwitchStmt* SwitchStmt::Get(Expr* cond, Stmt* block) {
  return new (SwitchStmtPool.Allocate()) SwitchStmt{cond, block};
}

AstNodeType SwitchStmt::Kind() const { return AstNodeType::kSwitchStmt; }

void SwitchStmt::Check() {
  if (!cond_->GetType()->IsIntegerTy()) {
    Error(cond_->GetLoc(), "switch quantity not an integer");
  }
}

SwitchStmt::SwitchStmt(Expr* cond, Stmt* block) : cond_{cond}, block_{block} {}

/*
 * WhileStmt
 */
WhileStmt* WhileStmt::Get(Expr* cond, Stmt* block) {
  return new (WhileStmtPool.Allocate()) WhileStmt{cond, block};
}

AstNodeType WhileStmt::Kind() const { return AstNodeType::kWhileStmt; }

void WhileStmt::Check() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(cond_->GetLoc(), "expect scalar");
  }
}

WhileStmt::WhileStmt(Expr* cond, Stmt* block) : cond_{cond}, block_{block} {}

/*
 * DoWhileStmt
 */
DoWhileStmt* DoWhileStmt::Get(Expr* cond, Stmt* block) {
  return new (DoWhileStmtPool.Allocate()) DoWhileStmt{cond, block};
}

AstNodeType DoWhileStmt::Kind() const { return AstNodeType::kDoWhileStmt; }

void DoWhileStmt::Check() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(cond_->GetLoc(), "expect scalar");
  }
}

DoWhileStmt::DoWhileStmt(Expr* cond, Stmt* block)
    : cond_{cond}, block_{block} {}

/*
 * ForStmt
 */
ForStmt* ForStmt::Get(Expr* init, Expr* cond, Expr* inc, Stmt* block,
                      Stmt* decl) {
  return new (ForStmtPool.Allocate()) ForStmt{init, cond, inc, block, decl};
}

AstNodeType ForStmt::Kind() const { return AstNodeType::kForStmt; }

void ForStmt::Check() {
  if (cond_ && !cond_->GetType()->IsScalarTy()) {
    Error(cond_->GetLoc(), "expect scalar");
  }
}

ForStmt::ForStmt(Expr* init, Expr* cond, Expr* inc, Stmt* block, Stmt* decl)
    : init_{init}, cond_{cond}, inc_{inc}, block_{block}, decl_{decl} {}

/*
 * GotoStmt
 */
GotoStmt* GotoStmt::Get(IdentifierExpr* ident) {
  return new (GotoStmtPool.Allocate()) GotoStmt{ident};
}

AstNodeType GotoStmt::Kind() const { return AstNodeType::kGotoStmt; }

void GotoStmt::Check() {}

GotoStmt::GotoStmt(IdentifierExpr* ident) : ident_{ident} {}

/*
 * ContinueStmt
 */
ContinueStmt* ContinueStmt::Get() {
  return new (ContinueStmtPool.Allocate()) ContinueStmt{};
}

AstNodeType ContinueStmt::Kind() const { return AstNodeType::kContinueStmt; }

void ContinueStmt::Check() {}

/*
 * BreakStmt
 */
BreakStmt* BreakStmt::Get() {
  return new (BreakStmtPool.Allocate()) BreakStmt{};
}

AstNodeType BreakStmt::Kind() const { return AstNodeType::kBreakStmt; }

void BreakStmt::Check() {}

/*
 * ReturnStmt
 */
ReturnStmt* ReturnStmt::Get(Expr* expr) {
  return new (ReturnStmtPool.Allocate()) ReturnStmt{expr};
}

AstNodeType ReturnStmt::Kind() const { return AstNodeType::kReturnStmt; }

void ReturnStmt::Check() {}

ReturnStmt::ReturnStmt(Expr* expr) : expr_{expr} {}

/*
 * TranslationUnit
 */
TranslationUnit* TranslationUnit::Get() {
  return new (TranslationUnitPool.Allocate()) TranslationUnit{};
}

AstNodeType TranslationUnit::Kind() const {
  return AstNodeType::kTranslationUnit;
}

void TranslationUnit::Check() {}

void TranslationUnit::AddExtDecl(ExtDecl* ext_decl) {
  // _Static_assert / e.g. int;
  if (ext_decl == nullptr) {
    return;
  }
  // 是声明而不是函数定义
  if (auto p{dynamic_cast<CompoundStmt*>(ext_decl)}; p) {
    for (const auto& decl : p->GetStmts()) {
      ext_decls_.push_back(decl);
    }
  } else {
    ext_decls_.push_back(ext_decl);
  }
}

std::vector<ExtDecl*> TranslationUnit::GetExtDecl() const { return ext_decls_; }

/*
 * Initializer
 */
Initializer::Initializer(
    Type* type, std::int32_t offset, Expr* expr,
    const std::list<std::pair<Type*, std::int32_t>>& indexs)
    : type_(type), offset_(offset), expr_(expr), indexs_{indexs} {}

Type* Initializer::GetType() const {
  assert(type_ != nullptr);
  return type_;
}

int32_t Initializer::GetOffset() const { return offset_; }

Expr* Initializer::GetExpr() const {
  assert(expr_ != nullptr);
  return expr_;
}

std::list<std::pair<Type*, std::int32_t>> Initializer::GetIndexs() const {
  return indexs_;
}

bool operator<(const Initializer& lhs, const Initializer& rhs) {
  return lhs.offset_ < rhs.offset_;
}

/*
 * Initializers
 */
void Initializers::AddInit(const Initializer& init) {
  for (auto& item : inits_) {
    if (item.offset_ == init.offset_) {
      item = init;
      return;
    }
  }

  inits_.push_back(init);
}

std::size_t Initializers::size() const { return std::size(inits_); }

/*
 * Declaration
 */
Declaration* Declaration::Get(IdentifierExpr* ident) {
  return new (DeclarationPool.Allocate()) Declaration{ident};
}

AstNodeType Declaration::Kind() const { return AstNodeType::kDeclaration; }

void Declaration::Check() {}

bool Declaration::HasLocalInit() const {
  return std::size(inits_) || value_init_;
}

void Declaration::AddInits(const Initializers& inits) {
  if (std::size(inits) == 0) {
    // GNU 扩展
    value_init_ = true;
    return;
  }

  inits_ = inits;

  std::sort(std::begin(inits_), std::end(inits_));

  // 必须用常量表达式初始化
  if (ident_->GetLinkage() != kNone) {
    for (auto&& item : inits_.inits_) {
      if (item.expr_->GetType()->IsIntegerTy()) {
        auto val{CalcExpr<std::uint64_t>{}.Calc(item.expr_)};
        item.expr_ = MakeNode<ConstantExpr>(item.expr_->GetLoc(),
                                            item.expr_->GetType(), val);
      } else if (item.expr_->GetType()->IsFloatPointTy()) {
        auto val{CalcExpr<long double>{}.Calc(item.expr_)};
        item.expr_ = MakeNode<ConstantExpr>(item.expr_->GetLoc(),
                                            item.expr_->GetType(), val);
      } else if (item.expr_->GetType()->IsAggregateTy()) {
      } else if (item.expr_->GetType()->IsPointerTy()) {
      } else {
        assert(false);
      }
    }
  }

  if (ident_->GetType()->IsScalarTy()) {
    assert(std::size(inits_) == 1);
    auto& init{*std::begin(inits_)};
    if (!init.type_->Equal(init.expr_->GetType())) {
      init.expr_ = Expr::MayCastTo(init.expr_, init.type_);
    }
  } else if (ident_->GetType()->IsAggregateTy()) {
    auto last{*(std::end(inits_) - 1)};

    // TODO 柔性数组怎么实现???
    if ((last.offset_ + last.type_->GetWidth() >
         ident_->GetType()->GetWidth()) &&
        !(ident_->GetType()->IsStructTy() &&
          ident_->GetType()->StructHasFlexibleArray())) {
      Error(loc_, "excess elements in array initializer");
    }

    for (auto& init : inits_) {
      if (!init.type_->Equal(init.expr_->GetType())) {
        init.expr_ = Expr::MayCastTo(init.expr_, init.type_);
      }
    }
  }
}

IdentifierExpr* Declaration::GetIdent() const { return ident_; }

bool Declaration::IsObjDecl() const {
  return dynamic_cast<ObjectExpr*>(ident_);
}

void Declaration::SetConstant(llvm::Constant* constant) {
  constant_ = constant;
}

llvm::Constant* Declaration::GetGlobalInit() const { return constant_; }

bool Declaration::IsObjDeclInGlobal() const {
  assert(IsObjDecl());
  return dynamic_cast<ObjectExpr*>(ident_)->InGlobal();
}

ObjectExpr* Declaration::GetObject() const {
  assert(IsObjDecl());
  return dynamic_cast<ObjectExpr*>(ident_);
}

bool Declaration::HasGlobalInit() const { return constant_ != nullptr; }

std::vector<Initializer> Declaration::GetLocalInits() const {
  return inits_.inits_;
}

Declaration::Declaration(IdentifierExpr* ident) : ident_{ident} {}

/*
 * FuncDef
 */
FuncDef* FuncDef::Get(IdentifierExpr* ident) {
  return new (FuncDefPool.Allocate()) FuncDef{ident};
}

AstNodeType FuncDef::Kind() const { return AstNodeType::kFuncDef; }

void FuncDef::Check() {
  for (const auto& param : ident_->GetQualType()->FuncGetParams()) {
    if (param->IsAnonymous()) {
      Error(param, "param name omitted");
    }
  }
}

void FuncDef::SetBody(CompoundStmt* body) {
  if (ident_->GetType()->IsComplete()) {
    Error(ident_->GetLoc(), "redefinition of func {}", ident_->GetName());
  } else {
    body_ = body;
    ident_->GetType()->SetComplete(true);
  }
}

std::string FuncDef::GetName() const { return ident_->GetName(); }

enum Linkage FuncDef::GetLinkage() const { return ident_->GetLinkage(); }

QualType FuncDef::GetFuncType() const { return ident_->GetType(); }

IdentifierExpr* FuncDef::GetIdent() const { return ident_; }

FuncDef::FuncDef(IdentifierExpr* ident) : ident_{ident} {}

}  // namespace kcc
