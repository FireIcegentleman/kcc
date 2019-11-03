//
// Created by kaiser on 2019/10/31.
//

#include "ast.h"

#include "visitor.h"

namespace kcc {

#define ACCEPT(ClassName) \
  void ClassName::Accept(Visitor& visitor) const { visitor.Visit(*this); }

ACCEPT(CompoundStmt)
ACCEPT(IfStmt)
ACCEPT(ReturnStmt)
ACCEPT(LabelStmt)
ACCEPT(UnaryOpExpr)
ACCEPT(BinaryOpExpr)
ACCEPT(ConditionOpExpr)
ACCEPT(TypeCastExpr)
ACCEPT(FuncCallExpr)
ACCEPT(Constant)
ACCEPT(Identifier)
ACCEPT(Enumerator)
ACCEPT(Object)
ACCEPT(TranslationUnit)
ACCEPT(JumpStmt)
ACCEPT(Declaration)
ACCEPT(FuncDef)

Expr::Expr(const Token& tok, std::shared_ptr<Type> type)
    : tok_{tok}, type_{type} {}

void Expr::EnsureCompatible(const std::shared_ptr<Type>& lhs,
                            const std::shared_ptr<Type>& rhs) {
  if (!lhs->Compatible(rhs)) {
    Error(tok_, "incompatible types: '{}' and '{}'", lhs->ToString(),
          rhs->ToString());
  }
}

std::shared_ptr<Type> Expr::GetType() const { return type_; }

Token Expr::GetToken() const { return tok_; }

void Expr::SetToken(const Token& tok) { tok_ = tok; }

bool Expr::IsConstQualified() const { return type_->IsConstQualified(); }

bool Expr::IsRestrictQualified() const { return type_->IsRestrictQualified(); }

bool Expr::IsVolatileQualified() const { return type_->IsVolatileQualified(); }

std::shared_ptr<Expr> Expr::MayCast(std::shared_ptr<Expr> expr) {
  auto type{Type::MayCast(expr->GetType())};

  if (!type->Equal(expr->GetType())) {
    return std::make_shared<TypeCastExpr>(expr, type);
  } else {
    return expr;
  }
}

std::shared_ptr<Expr> Expr::CastTo(std::shared_ptr<Expr> expr,
                                   std::shared_ptr<Type> type) {
  expr = MayCast(expr);
  return std::make_shared<TypeCastExpr>(expr, type);
  // TODO ???
}

void Expr::EnsureCompatibleOrVoidPtr(const std::shared_ptr<Type>& lhs,
                                     const std::shared_ptr<Type>& rhs) {
  if ((lhs->IsPointerTy() && rhs->IsPointerTy()) &&
      (lhs->GetPointerElementType()->IsVoidTy() ||
       rhs->GetPointerElementType()->IsVoidTy())) {
    return;
  }

  return EnsureCompatible(lhs, rhs);
}

BinaryOpExpr::BinaryOpExpr(const Token& tok, std::shared_ptr<Expr> lhs,
                           std::shared_ptr<Expr> rhs)
    : Expr(tok), op_(tok.GetTag()), lhs_(MayCast(lhs)), rhs_(MayCast(rhs)) {}

AstNodeType BinaryOpExpr::Kind() const { return AstNodeType::kBinaryOpExpr; }

bool BinaryOpExpr::IsLValue() const {
  // 在 C++ 中赋值运算符表达式是左值表达式，逗号运算符表达式可以是左值表达式
  // 而在 C 中两者都绝不是
  switch (op_) {
    case Tag::kLeftSquare:
      return true;
    case Tag::kPeriod:
      return lhs_->IsLValue();
    default:
      return false;
  }
}

void BinaryOpExpr::TypeCheck() {
  switch (op_) {
    case Tag::kEqual:
      AssignOpTypeCheck();
      break;
    case Tag::kExclaimEqual:
    case Tag::kEqualEqual:
      EqualityOpTypeCheck();
      break;
    case Tag::kLeftSquare:
      IndexOpTypeCheck();
      break;
    case Tag::kPlus:
      AddOpTypeCheck();
      break;
    case Tag::kMinus:
      SubOpTypeCheck();
      break;
    case Tag::kStar:
    case Tag::kPercent:
    case Tag::kSlash:
      MultiOpTypeCheck();
      break;
    case Tag::kComma:
      CommaOpTypeCheck();
      break;
    case Tag::kAmpAmp:
    case Tag::kPipePipe:
      LogicalOpTypeCheck();
      break;
    case Tag::kCaret:
    case Tag::kPipe:
      BitwiseOpTypeCheck();
      break;
    case Tag::kLessLess:
    case Tag::kGreaterGreater:
      ShiftOpTypeCheck();
      break;
    case Tag::kLess:
    case Tag::kLessEqual:
    case Tag::kGreater:
    case Tag::kGreaterEqual:
      RelationalOpTypeCheck();
      break;
    case Tag::kPeriod:
    case Tag::kArrow:
      MemberRefOpTypeCheck();
      break;
    default:
      assert(false);
  }
}

void BinaryOpExpr::IndexOpTypeCheck() {
  if (!lhs_->GetType()->IsPointerTy()) {
    Error(tok_.GetLoc(), "the operand should be integer type");
  }

  if (!rhs_->GetType()->IsIntegerTy()) {
    Error(tok_.GetLoc(), "the operand should be integer type");
  }

  if (lhs_->GetType()->GetPointerTo()->IsArrayTy()) {
    type_ = lhs_->GetType()->GetPointerTo()->GetArrayElementType();
  } else {
    type_ = lhs_->GetType()->GetPointerTo();
  }
}

void BinaryOpExpr::MemberRefOpTypeCheck() {
  // TODO ???
  type_ = rhs_->GetType();
}

void BinaryOpExpr::MultiOpTypeCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (op_ == Tag::kPercent) {
    if (!lhs_type->IsIntegerTy() || rhs_type->IsIntegerTy()) {
      Error(tok_.GetLoc(), "the operand should be integer type");
    }
  } else {
    if (!lhs_type->IsArithmeticTy() || rhs_type->IsArithmeticTy()) {
      Error(tok_.GetLoc(), "the operand should be integer type");
    }
  }

  type_ = Convert();
}

void BinaryOpExpr::AddOpTypeCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Convert();
  } else if (lhs_type->IsIntegerTy() && rhs_type->IsPointerTy()) {
    type_ = rhs_type;
    // 保持 lhs 是指针
    std::swap(lhs_, rhs_);
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    type_ = lhs_type;
  } else {
    Error(tok_.GetLoc(), "the operand should be integer type");
  }
}

void BinaryOpExpr::SubOpTypeCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Convert();
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    type_ = lhs_type;
  } else if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    // ptrdiff_t
    type_ = IntegerType::Get(64);
  } else {
    Error(tok_.GetLoc(), "the operand should be integer type");
  }
}

void BinaryOpExpr::ShiftOpTypeCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (!lhs_type->IsArithmeticTy() || rhs_type->IsArithmeticTy()) {
    Error(tok_.GetLoc(), "the operand should be integer type");
  }

  lhs_ = Expr::CastTo(lhs_, Type::IntegerPromote(lhs_type));
  rhs_ = Expr::CastTo(rhs_, Type::IntegerPromote(rhs_type));

  type_ = lhs_->GetType();
}

void BinaryOpExpr::RelationalOpTypeCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
  } else if (lhs_type->IsRealTy() && rhs_type->IsRealTy()) {
    Convert();
  } else {
    Error(tok_.GetLoc(), "the operand should be integer type");
  }

  type_ = IntegerType::Get(32);
}

void BinaryOpExpr::EqualityOpTypeCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
  } else if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    Convert();
  } else {
    Error(tok_.GetLoc(), "the operand should be integer type");
  }

  type_ = IntegerType::Get(32);
}

void BinaryOpExpr::BitwiseOpTypeCheck() {
  if (!lhs_->GetType()->IsIntegerTy() || !rhs_->GetType()->IsIntegerTy()) {
    Error(tok_.GetLoc(), "the operand should be integer type");
  }
  // 注意改变了 lhs rhs 的类型
  type_ = Convert();
}

void BinaryOpExpr::LogicalOpTypeCheck() {
  if (!lhs_->GetType()->IsScalarTy() || !rhs_->GetType()->IsScalarTy()) {
    Error(tok_.GetLoc(), "the operand should be scalar type");
  }
  type_ = IntegerType::Get(32);
}

void BinaryOpExpr::AssignOpTypeCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (lhs_->IsConstQualified()) {
    Error(lhs_->GetToken(), "left operand of '=' is const qualified: {}",
          lhs_type->ToString());
  } else if (!lhs_->IsLValue()) {
    Error(lhs_->GetToken(), "lvalue expression expected: {}",
          lhs_type->ToString());
  }

  if ((!lhs_type->IsArithmeticTy() || !rhs_type->IsArithmeticTy()) &&
      !(lhs_type->IsBoolTy() && rhs_type->IsPointerTy())) {
    // 注意， 目前 NULL 预处理之后为 (void*)0
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
  }

  rhs_ = Expr::CastTo(rhs_, lhs_type);
  type_ = lhs_type;
}

void BinaryOpExpr::CommaOpTypeCheck() { type_ = rhs_->GetType(); }

std::shared_ptr<Type> BinaryOpExpr::Convert() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  assert(lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy());

  auto type = Type::MaxType(lhs_type, rhs_type);

  if (!lhs_type->Equal(type)) {
    lhs_ = std::make_shared<TypeCastExpr>(lhs_, type);
  } else if (!rhs_type->Equal(type)) {
    rhs_ = std::make_shared<TypeCastExpr>(rhs_, type);
  }

  return type;
}

UnaryOpExpr::UnaryOpExpr(const Token& tok, std::shared_ptr<Expr> expr)
    : Expr(tok), op_(tok.GetTag()) {
  if (op_ == Tag::kAmp) {
    expr_ = expr;
  } else {
    expr_ = MayCast(expr);
  }
}

AstNodeType UnaryOpExpr::Kind() const { return AstNodeType::kUnaryOpExpr; }

bool UnaryOpExpr::IsLValue() const { return op_ == Tag::kStar; }

void UnaryOpExpr::TypeCheck() {
  switch (op_) {
    case Tag::kPlusPlus:
    case Tag::kMinusMinus:
      IncDecOpTypeCheck();
      break;
    case Tag::kPlus:
    case Tag::kMinus:
      UnaryAddSubOpTypeCheck();
      break;
    case Tag::kTilde:
      NotOpTypeCheck();
      break;
    case Tag::kExclaim:
      LogicNotOpTypeCheck();
      break;
    case Tag::kStar:
      DerefOpTypeCheck();
      break;
    case Tag::kAmp:
      AddrOpTypeCheck();
      break;
    default:
      assert(false);
  }
}

void UnaryOpExpr::IncDecOpTypeCheck() {
  auto expr_type{expr_->GetType()};

  if (expr_->IsConstQualified()) {
    Error(expr_->GetToken(), "left operand of '=' is const qualified: {}",
          expr_type->ToString());
  } else if (!expr_->IsLValue()) {
    Error(expr_->GetToken(), "lvalue expression expected: {}",
          expr_type->ToString());
  }

  if (expr_type->IsIntegerTy() || expr_type->IsRealFloatPointTy() ||
      expr_type->IsPointerTy()) {
    type_ = expr_type;
  } else {
    Error(expr_->GetToken(), "expect operand of real type or pointer");
  }
}

void UnaryOpExpr::UnaryAddSubOpTypeCheck() {
  if (!expr_->GetType()->IsArithmeticTy()) {
    Error(expr_->GetToken(), "expect operand of real type or pointer");
  }

  expr_ = std::make_shared<TypeCastExpr>(
      expr_, Type::IntegerPromote(expr_->GetType()));
  type_ = expr_->GetType();
}

void UnaryOpExpr::NotOpTypeCheck() {
  if (!expr_->GetType()->IsIntegerTy()) {
    Error(expr_->GetToken(), "expect operand of real type or pointer");
  }

  expr_ = std::make_shared<TypeCastExpr>(
      expr_, Type::IntegerPromote(expr_->GetType()));
  type_ = expr_->GetType();
}

void UnaryOpExpr::LogicNotOpTypeCheck() {
  if (!expr_->GetType()->IsScalarTy()) {
    Error(expr_->GetToken(), "expect operand of real type or pointer");
  }

  type_ = IntegerType::Get(32);
}

void UnaryOpExpr::DerefOpTypeCheck() {
  if (!expr_->GetType()->IsPointerTy()) {
    Error(expr_->GetToken(), "expect operand of real type or pointer");
  }

  type_ = expr_->GetType()->GetPointerElementType();
}

void UnaryOpExpr::AddrOpTypeCheck() {
  if (!expr_->GetType()->IsFunctionTy() && !expr_->IsLValue()) {
    Error(expr_->GetToken(), "expect operand of real type or pointer");
  }

  type_ = PointerType::Get(expr_->GetType());
}

AstNodeType TypeCastExpr::Kind() const { return AstNodeType::kTypeCastExpr; }

bool TypeCastExpr::IsLValue() const { return false; }

void TypeCastExpr::TypeCheck() {
  if (!to_->IsVoidTy() && !expr_->GetType()->IsScalarTy()) {
    Error(expr_->GetToken(), "expect operand of real type or pointer");
  }
  // TODO more check
  type_ = to_;
}

AstNodeType ConditionOpExpr::Kind() const {
  return AstNodeType::kConditionOpExpr;
}

bool ConditionOpExpr::IsLValue() const {
  // 和 C++ 不同，C 的条件运算符表达式绝不是左值
  return false;
}

void ConditionOpExpr::TypeCheck() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(tok_, "value of type '{}' is not contextually convertible to 'bool'",
          cond_->GetType()->ToString());
  }

  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};
  // TODO 处理得不正确
  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Convert();
  } else {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
    type_ = lhs_type;
  }
}

std::shared_ptr<Type> ConditionOpExpr::Convert() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  assert(lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy());

  auto type = Type::MaxType(lhs_type, rhs_type);

  if (!lhs_type->Equal(type)) {
    lhs_ = std::make_shared<TypeCastExpr>(lhs_, type);
  } else if (!rhs_type->Equal(type)) {
    rhs_ = std::make_shared<TypeCastExpr>(rhs_, type);
  }

  return type;
}

AstNodeType FuncCallExpr::Kind() const {
  return AstNodeType::kFunctionCallExpr;
}

bool FuncCallExpr::IsLValue() const { return false; }

void FuncCallExpr::TypeCheck() {
  if (callee_->GetType()->IsPointerTy()) {
    if (!callee_->GetType()->GetPointerElementType()->IsFunctionTy()) {
      Error(tok_.GetLoc(),
            "called object is not a function or function pointer");
    }
    type_ =
        callee_->GetType()->GetPointerElementType()->GetFunctionReturnType();
    callee_ = std::make_shared<UnaryOpExpr>(Token::Get(Tag::kStar), callee_);
  } else if (callee_->GetType()->IsFunctionTy()) {
    auto args_iter{std::begin(args_)};
    for (const auto& param : callee_->GetType()->GetFunctionParams()) {
      if (args_iter == std::end(args_)) {
        Error(tok_, "too few arguments for function call");
      }
      *args_iter = Expr::CastTo(*args_iter, param->GetType());
      ++args_iter;
    }

    if (args_iter != std::end(args_) &&
        !callee_->GetType()->IsFunctionVarArgs()) {
      Error(tok_, "too few arguments for function call");
    }

    while (args_iter != std::end(args_)) {
      if ((*args_iter)->GetType()->IsFloatTy()) {
        *args_iter =
            std::make_shared<TypeCastExpr>(*args_iter, Type::GetDoubleTy());
      } else if ((*args_iter)->GetType()->IsIntegerTy()) {
        *args_iter = std::make_shared<TypeCastExpr>(
            *args_iter, Type::IntegerPromote((*args_iter)->GetType()));
      }
      ++args_iter;
    }

    type_ = callee_->GetType()->GetFunctionReturnType();
  } else {
    Error(tok_.GetLoc(), "called object is not a function or function pointer");
  }
}

std::shared_ptr<Type> FuncCallExpr::GetFuncType() const {
  if (callee_->GetType()->IsPointerTy()) {
    return callee_->GetType()->GetPointerElementType();
  } else {
    return callee_->GetType();
  }
}

AstNodeType Constant::Kind() const { return AstNodeType::kConstantExpr; }

bool Constant::IsLValue() const { return false; }

void Constant::TypeCheck() {}

long Constant::GetIntVal() const { return int_val_; }

double Constant::GetFloatVal() const { return float_val_; }

std::string Constant::GetStrVal() const { return str_val_; }

AstNodeType Identifier::Kind() const { return AstNodeType::kIdentifier; }

bool Identifier::IsLValue() const { return false; }

void Identifier::TypeCheck() {}

enum Linkage Identifier::GetLinkage() const { return linkage_; }

void Identifier::SetLinkage(enum Linkage linkage) { linkage_ = linkage; }

std::string Identifier::GetName() const { return tok_.GetStr(); }

bool Identifier::IsTypeName() const { return is_type_name_; }

bool Identifier::IsObject() const { return dynamic_cast<const Object*>(this); }

bool Identifier::IsEnumerator() const {
  return dynamic_cast<const Enumerator*>(this);
}

Enumerator::Enumerator(const Token& tok, std::int32_t val)
    : Identifier{tok, IntegerType::Get(32), kNone, false},
      val_{std::make_shared<Constant>(tok, val)} {}

AstNodeType Enumerator::Kind() const { return AstNodeType::kEnumerator; }

bool Enumerator::IsLValue() const { return false; }

void Enumerator::TypeCheck() {}

std::int32_t Enumerator::GetVal() const { return val_->GetIntVal(); }

AstNodeType Object::Kind() const { return AstNodeType::kObject; }

bool Object::IsLValue() const { return true; }

void Object::TypeCheck() {}

bool Object::IsStatic() const { return type_->IsStatic(); }

std::int32_t Object::GetAlign() const { return type_->GetAlign(); }

std::int32_t Object::GetOffset() const { return offset_; }

void Object::SetOffset(std::int32_t offset) { offset_ = offset; }

std::shared_ptr<Declaration> Object::GetDecl() { return decl_; }

void Object::SetDecl(std::shared_ptr<Declaration> decl) { decl_ = decl; }

bool Object::HasInit() const { return decl_->HasInit(); }

bool Object::Anonymous() const { return anonymous_; }

AstNodeType FuncDef::Kind() const { return AstNodeType::kFuncDef; }

void FuncDef::SetBody(std::shared_ptr<CompoundStmt> body) { body_ = body; }

std::string FuncDef::GetName() const { return ident_->GetName(); }

enum Linkage FuncDef::GetLinkage() { return ident_->GetLinkage(); }

std::shared_ptr<Type> FuncDef::GetFuncType() const { return ident_->GetType(); }

AstNodeType TranslationUnit::Kind() const {
  return AstNodeType::kTranslationUnit;
}

LabelStmt::LabelStmt(const std::string& label) : label_{label} {}

AstNodeType LabelStmt::Kind() const { return AstNodeType::kLabelStmt; }

IfStmt::IfStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> then_block,
               std::shared_ptr<Stmt> else_block)
    : cond_{cond}, then_block_{then_block}, else_block_{else_block} {}

AstNodeType IfStmt::Kind() const { return AstNodeType::kIfStmt; }

JumpStmt::JumpStmt(std::shared_ptr<LabelStmt> label) : label_{label} {}

AstNodeType JumpStmt::Kind() const { return AstNodeType::kJumpStmt; }

ReturnStmt::ReturnStmt(std::shared_ptr<Expr> expr) : expr_{expr} {}

AstNodeType ReturnStmt::Kind() const { return AstNodeType::kReturnStmt; }

CompoundStmt::CompoundStmt(std::vector<std::shared_ptr<Stmt>> stmts)
    : stmts_{std::move(stmts)} {}

AstNodeType CompoundStmt::Kind() const { return AstNodeType::kCompoundStmt; }

std::shared_ptr<Scope> CompoundStmt::GetScope() { return scope_; }

void CompoundStmt::AddStmt(std::shared_ptr<Stmt> stmt) {
  stmts_.push_back(stmt);
}

AstNodeType Declaration::Kind() const { return AstNodeType::kDeclaration; }

void Declaration::AddInit(const Initializer& init) { inits_.insert(init); }

bool Declaration::HasInit() const { return std::size(inits_); }

bool operator<(const Initializer& lhs, const Initializer& rhs) {
  return lhs.offset_ < rhs.offset_;
}

}  // namespace kcc
