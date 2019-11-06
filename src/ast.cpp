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
ACCEPT(Declaration)
ACCEPT(FuncDef)
ACCEPT(ExprStmt)
ACCEPT(WhileStmt)
ACCEPT(DoWhileStmt)
ACCEPT(ForStmt)
ACCEPT(CaseStmt)
ACCEPT(DefaultStmt)
ACCEPT(SwitchStmt)
ACCEPT(GotoStmt)
ACCEPT(ContinueStmt)
ACCEPT(BreakStmt)

/*
 * Expr
 */
Expr::Expr(const Token& tok, QualType type) : tok_{tok}, type_{type} {}

Token Expr::GetToken() const { return tok_; }

void Expr::SetToken(const Token& tok) { tok_ = tok; }

QualType Expr::GetQualType() const { return type_; }

std::shared_ptr<Type> Expr::GetType() const { return type_.GetType(); }

bool Expr::IsConst() const { return type_.IsConst(); }

bool Expr::IsRestrict() const { return type_.IsRestrict(); }

void Expr::EnsureCompatible(QualType lhs, QualType rhs) const {
  if (!lhs->Compatible(rhs.GetType())) {
    Error(tok_, "incompatible types: '{}' vs '{}'", lhs->ToString(),
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

std::shared_ptr<Expr> Expr::MayCast(const std::shared_ptr<Expr>& expr) {
  auto type{Type::MayCast(expr->GetQualType())};

  if (type != expr->GetQualType()) {
    return MakeAstNode<TypeCastExpr>(expr, type);
  } else {
    return expr;
  }
}

std::shared_ptr<Expr> Expr::MayCastTo(std::shared_ptr<Expr> expr, QualType to) {
  expr = MayCast(expr);

  if (!expr->GetQualType()->Equal(to.GetType())) {
    return MakeAstNode<TypeCastExpr>(expr, to);
  } else {
    return expr;
  }
}

/*
 * BinaryOpExpr
 */
BinaryOpExpr::BinaryOpExpr(const Token& tok, Tag tag, std::shared_ptr<Expr> lhs,
                           std::shared_ptr<Expr> rhs)
    : Expr(tok), op_(tag), lhs_(MayCast(lhs)), rhs_(MayCast(rhs)) {}

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
    case Tag::kLeftSquare:
      IndexOpCheck();
      break;
    case Tag::kPeriod:
    case Tag::kArrow:
      MemberRefOpCheck();
      break;
    case Tag::kComma:
      CommaOpCheck();
      break;
    default:
      assert(false);
  }
}

std::shared_ptr<Type> BinaryOpExpr::Convert() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  auto type = ArithmeticType::MaxType(lhs_type.GetType(), rhs_type.GetType());

  Expr::MayCastTo(lhs_, type);
  Expr::MayCastTo(rhs_, type);

  return type;
}

void BinaryOpExpr::AssignOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_->IsConst()) {
    Error(lhs_->GetToken(), "left operand of '=' is const qualified: {}",
          lhs_type->ToString());
  } else if (!lhs_->IsLValue()) {
    Error(lhs_->GetToken(), "'=': lvalue expression expected: {}",
          lhs_type->ToString());
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
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = QualType{Convert()};
  } else if (lhs_type->IsIntegerTy() && rhs_type->IsPointerTy()) {
    type_ = rhs_type;
    // 保持 lhs 是指针
    std::swap(lhs_, rhs_);
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    type_ = lhs_type;
  } else {
    Error(tok_,
          "'+' the operand should be integer or pointer type '{}' vs '{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }
}

void BinaryOpExpr::SubOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = QualType{Convert()};
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    type_ = lhs_type;
  } else if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    // ptrdiff_t
    type_ = QualType{ArithmeticType::Get(64)};
  } else {
    Error(tok_,
          "'+' the operand should be integer or pointer type '{}' vs '{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }
}

void BinaryOpExpr::MultiOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (op_ == Tag::kPercent) {
    if (!lhs_type->IsIntegerTy() || !rhs_type->IsIntegerTy()) {
      Error(tok_, "'%': the operand should be integer type '{}' vs '{}'",
            lhs_type->ToString(), rhs_type->ToString());
    }
  } else {
    if (!lhs_type->IsArithmeticTy() || !rhs_type->IsArithmeticTy()) {
      Error(tok_,
            "'*' or '/': the operand should be arithmetic type '{}' vs '{}'",
            lhs_type->ToString(), rhs_type->ToString());
    }
  }

  type_ = QualType{Convert()};
}

void BinaryOpExpr::BitwiseOpCheck() {
  if (!lhs_->GetQualType()->IsIntegerTy() ||
      !rhs_->GetQualType()->IsIntegerTy()) {
    Error(tok_,
          "'&' / '|' / '^': the operand should be arithmetic type '{}' vs '{}'",
          lhs_->GetQualType()->ToString(), rhs_->GetQualType()->ToString());
  }

  type_ = QualType{Convert()};
}

void BinaryOpExpr::ShiftOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (!lhs_type->IsIntegerTy() || !rhs_type->IsIntegerTy()) {
    Error(tok_.GetLoc(),
          "'<<' or '>>': the operand should be arithmetic type '{}' vs '{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }

  lhs_ =
      Expr::MayCastTo(lhs_, ArithmeticType::IntegerPromote(lhs_type.GetType()));
  rhs_ =
      Expr::MayCastTo(rhs_, ArithmeticType::IntegerPromote(rhs_type.GetType()));

  type_ = lhs_->GetQualType();
}

void BinaryOpExpr::LogicalOpCheck() {
  if (!lhs_->GetQualType()->IsScalarTy() ||
      !rhs_->GetQualType()->IsScalarTy()) {
    Error(tok_, "'&&' or '||': the operand should be scalar type '{}' vs '{}'",
          lhs_->GetQualType()->ToString(), rhs_->GetQualType()->ToString());
  }

  type_ = QualType{ArithmeticType::Get(32)};
}

void BinaryOpExpr::EqualityOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
  } else if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    Convert();
  } else {
    Error(tok_, "'==' or '!=': the operand should be integer type '{}' vs '{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }

  type_ = QualType{ArithmeticType::Get(32)};
}

void BinaryOpExpr::RelationalOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
  } else if (lhs_type->IsRealTy() && rhs_type->IsRealTy()) {
    Convert();
  } else {
    Error(tok_.GetLoc(),
          "'<' / '>' / '<=' / '>=': the operand should be integer type '{}' vs "
          "'{}'",
          lhs_type->ToString(), rhs_type->ToString());
  }

  type_ = QualType{ArithmeticType::Get(32)};
}

void BinaryOpExpr::IndexOpCheck() {
  if (!lhs_->GetQualType()->IsPointerTy()) {
    Error(tok_, "'[]': the operand should be pointer type: '{}'",
          lhs_->GetQualType()->ToString());
  }

  if (!rhs_->GetQualType()->IsIntegerTy()) {
    Error(tok_, "'[]': the operand should be integer type: '{}'",
          lhs_->GetQualType()->ToString());
  }

  if (lhs_->GetQualType()->PointerGetElementType()->IsArrayTy()) {
    type_ = lhs_->GetQualType()->PointerGetElementType()->ArrayGetElementType();
  } else {
    type_ = QualType{lhs_->GetQualType()->PointerGetElementType()};
  }
}

void BinaryOpExpr::MemberRefOpCheck() { type_ = rhs_->GetQualType(); }

void BinaryOpExpr::CommaOpCheck() { type_ = rhs_->GetQualType(); }

/*
 * UnaryOpExpr
 */
UnaryOpExpr::UnaryOpExpr(const Token& tok, Tag tag, std::shared_ptr<Expr> expr)
    : Expr(tok), op_(tag) {
  if (op_ == Tag::kAmp) {
    expr_ = expr;
  } else {
    expr_ = MayCast(expr);
  }
}

AstNodeType UnaryOpExpr::Kind() const { return AstNodeType::kUnaryOpExpr; }

bool UnaryOpExpr::IsLValue() const { return op_ == Tag::kStar; }

void UnaryOpExpr::Check() {
  switch (op_) {
    case Tag::kPlusPlus:
    case Tag::kMinusMinus:
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

void UnaryOpExpr::IncDecOpCheck() {
  auto expr_type{expr_->GetQualType()};

  if (expr_->IsConst()) {
    Error(expr_->GetToken(),
          "'++' / '--': left operand of '=' is const qualified: {}",
          expr_type->ToString());
  } else if (!expr_->IsLValue()) {
    Error(expr_->GetToken(), "'++' / '--': lvalue expression expected: {}",
          expr_type->ToString());
  }

  if (expr_type->IsIntegerTy() || expr_type->IsRealFloatPointTy() ||
      expr_type->IsPointerTy()) {
    type_ = expr_type;
  } else {
    Error(expr_->GetToken(), "expect operand of real type or pointer");
  }
}

void UnaryOpExpr::UnaryAddSubOpCheck() {
  if (!expr_->GetQualType()->IsArithmeticTy()) {
    Error(expr_->GetToken(), "'+' / '-': expect operand of arithmetic type: {}",
          expr_->GetQualType()->ToString());
  }

  auto new_type{ArithmeticType::IntegerPromote(expr_->GetQualType().GetType())};
  Expr::MayCastTo(expr_, new_type);

  type_ = expr_->GetQualType();
}

void UnaryOpExpr::NotOpCheck() {
  if (!expr_->GetQualType()->IsIntegerTy()) {
    Error(expr_->GetToken(), "'~': expect operand of arithmetic type: {}",
          expr_->GetQualType()->ToString());
  }

  auto new_type{ArithmeticType::IntegerPromote(expr_->GetQualType().GetType())};
  Expr::MayCastTo(expr_, new_type);

  type_ = expr_->GetQualType();
}

void UnaryOpExpr::LogicNotOpCheck() {
  if (!expr_->GetQualType()->IsScalarTy()) {
    Error(expr_->GetToken(), "'~': expect operand of scalar type: {}",
          expr_->GetQualType()->ToString());
  }

  type_ = QualType{ArithmeticType::Get(32)};
}

void UnaryOpExpr::DerefOpCheck() {
  if (!expr_->GetQualType()->IsPointerTy()) {
    Error(expr_->GetToken(), "'~': expect operand of pointer type: {}",
          expr_->GetQualType()->ToString());
  }

  type_ = expr_->GetQualType()->PointerGetElementType();
}

void UnaryOpExpr::AddrOpCheck() {
  if (!expr_->GetQualType()->IsFunctionTy() && !expr_->IsLValue()) {
    Error(expr_->GetToken(),
          "'&': expression must be an lvalue or function: {}",
          expr_->GetQualType()->ToString());
  }

  type_ = QualType{PointerType::Get(expr_->GetQualType())};
}

/*
 * TypeCastExpr
 */
TypeCastExpr::TypeCastExpr(std::shared_ptr<Expr> expr, QualType to)
    : Expr(expr->GetToken(), to), expr_{expr}, to_{to} {}

AstNodeType TypeCastExpr::Kind() const { return AstNodeType::kTypeCastExpr; }

bool TypeCastExpr::IsLValue() const { return false; }

void TypeCastExpr::Check() {
  // TODO more check
}

/*
 * ConditionOpExpr
 */
AstNodeType ConditionOpExpr::Kind() const {
  return AstNodeType::kConditionOpExpr;
}

bool ConditionOpExpr::IsLValue() const {
  // 和 C++ 不同，C 的条件运算符表达式绝不是左值
  return false;
}

void ConditionOpExpr::Check() {
  if (!cond_->GetQualType()->IsScalarTy()) {
    Error(tok_, "value of type '{}' is not contextually convertible to 'bool'",
          cond_->GetQualType()->ToString());
  }

  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};
  // TODO 处理得不正确
  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Convert();
  } else {
    EnsureCompatibleOrVoidPtr(lhs_type, rhs_type);
    type_ = lhs_type;
  }
}

std::shared_ptr<Type> ConditionOpExpr::Convert() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  auto type = ArithmeticType::MaxType(lhs_type.GetType(), rhs_type.GetType());

  Expr::MayCastTo(lhs_, type);
  Expr::MayCastTo(rhs_, type);

  return type;
}

/*
 * FuncCallExpr
 */
FuncCallExpr::FuncCallExpr(std::shared_ptr<Expr> callee,
                           std::vector<std::shared_ptr<Expr>> args)
    : Expr{callee->GetToken()}, callee_{callee}, args_{std::move(args)} {}

AstNodeType FuncCallExpr::Kind() const {
  return AstNodeType::kFunctionCallExpr;
}

bool FuncCallExpr::IsLValue() const { return false; }

void FuncCallExpr::Check() {
  if (callee_->GetQualType()->IsPointerTy()) {
    if (!callee_->GetQualType()->PointerGetElementType()->IsFunctionTy()) {
      Error(tok_, "called object is not a function or function pointer: {}",
            callee_->GetQualType()->ToString());
    }

    callee_ =
        MakeAstNode<UnaryOpExpr>(callee_->GetToken(), Tag::kStar, callee_);
  }

  auto func_type{callee_->GetQualType().GetType()};
  if (!func_type) {
    Error(tok_, "called object is not a function or function pointer: {}",
          callee_->GetQualType()->ToString());
  }

  auto args_iter{std::begin(args_)};

  for (const auto& param : callee_->GetQualType()->FuncGetParams()) {
    if (args_iter == std::end(args_)) {
      Error(tok_, "too few arguments for function call");
    }
    *args_iter = Expr::MayCastTo(*args_iter, param->GetQualType());
    ++args_iter;
  }

  if (args_iter != std::end(args_) &&
      !callee_->GetQualType()->FuncIsVarArgs()) {
    Error(tok_, "too many arguments for function call");
  }

  while (args_iter != std::end(args_)) {
    // 浮点提升
    if ((*args_iter)->GetQualType()->IsFloatTy()) {
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

std::shared_ptr<Type> FuncCallExpr::GetFuncType() const {
  if (callee_->GetQualType()->IsPointerTy()) {
    return callee_->GetQualType()->PointerGetElementType().GetType();
  } else {
    return callee_->GetQualType().GetType();
  }
}

/*
 * Constant
 */
AstNodeType Constant::Kind() const { return AstNodeType::kConstantExpr; }

bool Constant::IsLValue() const { return false; }

void Constant::Check() {}

long Constant::GetIntegerVal() const { return integer_val_; }

double Constant::GetFloatPointVal() const { return float_point_val_; }

std::string Constant::GetStrVal() const { return str_val_; }

/*
 * Identifier
 */
Identifier::Identifier(const Token& tok, QualType type, enum Linkage linkage,
                       bool is_type_name)
    : Expr{tok, type}, linkage_{linkage}, is_type_name_{is_type_name} {}

AstNodeType Identifier::Kind() const { return AstNodeType::kIdentifier; }

bool Identifier::IsLValue() const { return false; }

void Identifier::Check() {}

enum Linkage Identifier::GetLinkage() const { return linkage_; }

void Identifier::SetLinkage(enum Linkage linkage) { linkage_ = linkage; }

std::string Identifier::GetName() const { return tok_.GetStr(); }

bool Identifier::IsTypeName() const { return is_type_name_; }

bool Identifier::IsObject() const { return dynamic_cast<const Object*>(this); }

bool Identifier::IsEnumerator() const {
  return dynamic_cast<const Enumerator*>(this);
}

/*
 * Enumerator
 */
Enumerator::Enumerator(const Token& tok, std::int32_t val)
    : Identifier{tok, QualType{ArithmeticType::Get(32)}, kNone, false},
      val_{val} {}

AstNodeType Enumerator::Kind() const { return AstNodeType::kEnumerator; }

bool Enumerator::IsLValue() const { return false; }

void Enumerator::Check() {}

std::int32_t Enumerator::GetVal() const { return val_; }

/*
 * Object
 */
Object::Object(const Token& tok, QualType type,
               std::uint32_t storage_class_spec, enum Linkage linkage,
               bool anonymous)
    : Identifier{tok, type, linkage, false},
      anonymous_{anonymous},
      storage_class_spec_{storage_class_spec},
      align_{type->GetAlign()} {}

AstNodeType Object::Kind() const { return AstNodeType::kObject; }

bool Object::IsLValue() const { return true; }

void Object::Check() {}

bool Object::IsStatic() const {
  return storage_class_spec_ & kStatic || GetLinkage() != kNone;
}

void Object::SetStorageClassSpec(std::uint32_t storage_class_spec) {
  storage_class_spec_ = storage_class_spec;
}

std::int32_t Object::GetAlign() const { return align_; }

void Object::SetAlign(std::int32_t align) { align_ = align; }

std::int32_t Object::GetOffset() const { return offset_; }

void Object::SetOffset(std::int32_t offset) { offset_ = offset; }

std::shared_ptr<Declaration> Object::GetDecl() { return decl_; }

void Object::SetDecl(std::shared_ptr<Declaration> decl) { decl_ = decl; }

bool Object::HasInit() const { return decl_->HasInit(); }

bool Object::Anonymous() const { return anonymous_; }

AstNodeType FuncDef::Kind() const { return AstNodeType::kFuncDef; }

/*
 * FuncDef
 */
void FuncDef::SetBody(std::shared_ptr<CompoundStmt> body) {
  if (ident_->GetType()->IsComplete()) {
    Error(ident_->GetToken(), "redefinition of func {}", ident_->GetName());
  } else {
    body_ = body;
    ident_->GetType()->SetComplete(true);
  }
}

std::string FuncDef::GetName() const { return ident_->GetName(); }

enum Linkage FuncDef::GetLinkage() const { return ident_->GetLinkage(); }

QualType FuncDef::GetFuncType() const { return ident_->GetQualType(); }

void FuncDef::Check() {
  for (const auto& param : ident_->GetQualType()->FuncGetParams()) {
    if (param->Anonymous()) {
      Error(param->GetToken(), "param name omitted");
    }
  }
}

/*
 * TranslationUnit
 */
AstNodeType TranslationUnit::Kind() const {
  return AstNodeType::kTranslationUnit;
}

void TranslationUnit::AddExtDecl(std::shared_ptr<ExtDecl> ext_decl) {
  // _Static_assert / e.g. int;
  if (ext_decl == nullptr) {
    return;
  }
  // 是声明而不是函数定义
  if (auto p{std::dynamic_pointer_cast<CompoundStmt>(ext_decl)}; p) {
    for (const auto& decl : p->GetStmts()) {
      ext_decls_.push_back(decl);
    }
  } else {
    ext_decls_.push_back(ext_decl);
  }
}

void TranslationUnit::Check() {}

/*
 * LabelStmt
 */
LabelStmt::LabelStmt(std::shared_ptr<Identifier> ident) : ident_{ident} {}

AstNodeType LabelStmt::Kind() const { return AstNodeType::kLabelStmt; }

/*
 * IfStmt
 */
IfStmt::IfStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> then_block,
               std::shared_ptr<Stmt> else_block)
    : cond_{cond}, then_block_{then_block}, else_block_{else_block} {}

AstNodeType IfStmt::Kind() const { return AstNodeType::kIfStmt; }

/*
 * ReturnStmt
 */
ReturnStmt::ReturnStmt(std::shared_ptr<Expr> expr) : expr_{expr} {}

AstNodeType ReturnStmt::Kind() const { return AstNodeType::kReturnStmt; }

void ReturnStmt::Check() {}

/*
 * CompoundStmt
 */
CompoundStmt::CompoundStmt(std::vector<std::shared_ptr<Stmt>> stmts,
                           std::shared_ptr<Scope> scope)
    : stmts_{std::move(stmts)}, scope_{scope} {}

AstNodeType CompoundStmt::Kind() const { return AstNodeType::kCompoundStmt; }

std::shared_ptr<Scope> CompoundStmt::GetScope() { return scope_; }

std::vector<std::shared_ptr<Stmt>> CompoundStmt::GetStmts() { return stmts_; }

void CompoundStmt::AddStmt(std::shared_ptr<Stmt> stmt) {
  // 非 typedef
  if (stmt) {
    stmts_.push_back(stmt);
  }
}

/*
 * Declaration
 */
AstNodeType Declaration::Kind() const { return AstNodeType::kDeclaration; }

void Declaration::AddInit(const Initializer& init) { inits_.insert(init); }

bool Declaration::HasInit() const { return std::size(inits_); }

void Declaration::Check() {}

bool operator<(const Initializer& lhs, const Initializer& rhs) {
  return lhs.offset_ < rhs.offset_;
}

/*
 * BreakStmt
 */
AstNodeType BreakStmt::Kind() const { return AstNodeType::kBreakStmt; }

/*
 * ContinueStmt
 */
AstNodeType ContinueStmt::Kind() const { return AstNodeType::kContinueStmt; }

/*
 * GotoStmt
 */
AstNodeType GotoStmt::Kind() const { return AstNodeType::kGotoStmt; }

void GotoStmt::Check() {}

/*
 * ExprStmt
 */
ExprStmt::ExprStmt(std::shared_ptr<Expr> expr) : expr_{expr} {}

AstNodeType ExprStmt::Kind() const { return AstNodeType::kExprStmt; }

void ExprStmt::Check() {}

/*
 * WhileStmt
 */
WhileStmt::WhileStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> block)
    : cond_{cond}, block_{block} {}

AstNodeType WhileStmt::Kind() const { return AstNodeType::kWhileStmt; }

void WhileStmt::Check() {}

/*
 * DoWhileStmt
 */
DoWhileStmt::DoWhileStmt(std::shared_ptr<Expr> cond,
                         std::shared_ptr<Stmt> block)
    : cond_{cond}, block_{block} {}

AstNodeType DoWhileStmt::Kind() const { return AstNodeType::kDoWhileStmt; }

void DoWhileStmt::Check() {}

/*
 * ForStmt
 */
ForStmt::ForStmt(std::shared_ptr<Expr> init, std::shared_ptr<Expr> cond,
                 std::shared_ptr<Expr> inc, std::shared_ptr<Stmt> block,
                 std::shared_ptr<Stmt> decl)
    : init_{init}, cond_{cond}, inc_{inc}, block_{block}, decl_{decl} {}

AstNodeType ForStmt::Kind() const { return AstNodeType::kForStmt; }

void ForStmt::Check() {}

/*
 * CaseStmt
 */
CaseStmt::CaseStmt(std::int32_t case_value, std::shared_ptr<Stmt> block)
    : case_value_{case_value}, block_{block} {}

CaseStmt::CaseStmt(std::int32_t case_value, std::int32_t case_value2,
                   std::shared_ptr<Stmt> block)
    : case_value_range_{case_value, case_value2},
      has_range_{true},
      block_{block} {}

AstNodeType CaseStmt::Kind() const { return AstNodeType::kCaseStmt; }

void CaseStmt::Check() {}

/*
 * DefaultStmt
 */
AstNodeType DefaultStmt::Kind() const { return AstNodeType::kDefaultStmt; }

DefaultStmt::DefaultStmt(std::shared_ptr<Stmt> block) : block_{block} {}

void DefaultStmt::Check() {}

/*
 * SwitchStmt
 */
SwitchStmt::SwitchStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> block)
    : cond_{cond}, block_{block} {}

AstNodeType SwitchStmt::Kind() const { return AstNodeType::kSwitchStmt; }

void ContinueStmt::Check() {}

void BreakStmt::Check() {}

void SwitchStmt::Check() {}

void CompoundStmt::Check() {}

void IfStmt::Check() {}

void LabelStmt::Check() {}

}  // namespace kcc
