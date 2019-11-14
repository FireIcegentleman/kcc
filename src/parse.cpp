//
// Created by kaiser on 2019/10/31.
//

#include "parse.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "calc.h"
#include "encoding.h"
#include "error.h"
#include "lex.h"

#ifdef DEV
#include "util.h"
#endif

namespace kcc {

Parser::Parser(std::vector<Token> tokens) : tokens_{std::move(tokens)} {}

TranslationUnit* Parser::ParseTranslationUnit() {
  while (HasNext()) {
    unit_->AddExtDecl(ParseExternalDecl());
  }

  if (SymbolTable) {
    curr_scope_->PrintCurrScope();
  }

  return unit_;
}

bool Parser::HasNext() { return !Peek().TagIs(Tag::kEof); }

Token Parser::Peek() { return tokens_[index_]; }

Token Parser::Next() { return tokens_[index_++]; }

void Parser::PutBack() {
  assert(index_ > 0);
  --index_;
}

bool Parser::Test(Tag tag) { return Peek().TagIs(tag); }

bool Parser::Try(Tag tag) {
  if (Test(tag)) {
    Next();
    return true;
  } else {
    return false;
  }
}

Token Parser::Expect(Tag tag) {
  if (!Test(tag)) {
    Error(tag, Peek());
  } else {
    return Next();
  }
}

void Parser::MarkLoc() { loc_ = Peek().GetLoc(); }

void Parser::EnterBlock(Type* func_type) {
  curr_scope_ = Scope::Get(curr_scope_, kBlock);

  if (func_type) {
    for (const auto& param : func_type->FuncGetParams()) {
      curr_scope_->InsertNormal(param->GetName(), param);
    }
  }
}

void Parser::ExitBlock() { curr_scope_ = curr_scope_->GetParent(); }

void Parser::EnterFunc(IdentifierExpr* ident) {
  curr_func_def_ = MakeAstNode<FuncDef>(ident);
}

void Parser::ExitFunc() { curr_func_def_ = nullptr; }

void Parser::EnterProto() { curr_scope_ = Scope::Get(curr_scope_, kFuncProto); }

void Parser::ExitProto() { curr_scope_ = curr_scope_->GetParent(); }

bool Parser::IsTypeName(const Token& tok) {
  if (tok.IsTypeSpecQual()) {
    return true;
  } else if (tok.IsIdentifier()) {
    auto ident{curr_scope_->FindNormal(tok)};
    if (ident && ident->IsTypeName()) {
      return true;
    }
  }

  return false;
}

bool Parser::IsDecl(const Token& tok) {
  if (tok.IsDecl()) {
    return true;
  } else if (tok.IsIdentifier()) {
    auto ident{curr_scope_->FindNormal(tok)};
    if (ident && ident->IsTypeName()) {
      return true;
    }
  }

  return false;
}

Declaration* Parser::MakeDeclaration(const std::string& name, QualType type,
                                     std::uint32_t storage_class_spec,
                                     std::uint32_t func_spec,
                                     std::int32_t align) {
  if (storage_class_spec & kTypedef) {
    if (align > 0) {
      Error(loc_, "'_Alignas' attribute applies to typedef");
    }

    auto ident{curr_scope_->FindNormalInCurrScope(name)};
    if (ident) {
      // 如果两次定义的类型兼容是可以的
      if (!type->Compatible(ident->GetType())) {
        Error(loc_, "typedef redefinition with different types '{}' vs '{}'",
              type->ToString(), ident->GetType()->ToString());
      } else {
        Warning(loc_, "Typedef redefinition");
        return nullptr;
      }
    } else {
      curr_scope_->InsertNormal(
          name, MakeAstNode<IdentifierExpr>(name, type, kNone, true));

      return nullptr;
    }
  } else if (storage_class_spec & kRegister) {
    if (align > 0) {
      Error(loc_, "'_Alignas' attribute applies to register");
    }
  }

  if (type->IsVoidTy()) {
    Error(loc_, "variable or field '{}' declared void", name);
  } else if (type->IsFunctionTy() && !curr_scope_->IsFileScope()) {
    Error(loc_, "function declaration is not allowed here");
  }

  Linkage linkage;
  if (curr_scope_->IsFileScope()) {
    if (storage_class_spec & kStatic) {
      linkage = kInternal;
    } else {
      linkage = kExternal;
    }
  } else {
    linkage = kNone;
  }

  auto ident{curr_scope_->FindNormalInCurrScope(name)};
  //有链接对象(外部或内部)的声明可以重复
  if (ident) {
    if (!type->Compatible(ident->GetType())) {
      Error(loc_, "conflicting types '{}' vs '{}'", type->ToString(),
            ident->GetType()->ToString());
    }

    if (linkage == kNone) {
      Error(loc_, "redefinition of '{}'", name);
    } else if (linkage == kExternal) {
      // static int a = 1;
      // extern int a;
      // 这种情况是可以的
      if (ident->GetLinkage() == kNone) {
        Error(loc_, "conflicting linkage '{}'", name);
      } else {
        linkage = ident->GetLinkage();
      }
    } else {
      if (ident->GetLinkage() != kInternal) {
        Error(loc_, "conflicting linkage '{}'", name);
      }
    }

    // extern int a;
    // int a = 1;
    if (auto p{dynamic_cast<ObjectExpr*>(ident)}; p) {
      if (storage_class_spec == 0) {
        p->SetStorageClassSpec(p->GetStorageClassSpec() & ~kExtern);
      }
    }
  }

  IdentifierExpr* ret;
  if (type->IsFunctionTy()) {
    if (align > 0) {
      Error(loc_, "'_Alignas' attribute applies to func");
    } else if (func_spec != 0) {
      type->FuncSetFuncSpec(func_spec);
    }

    ret = MakeAstNode<IdentifierExpr>(name, type, linkage, false);
  } else {
    ret =
        MakeAstNode<ObjectExpr>(name, type, storage_class_spec, linkage, false);
    if (align > 0) {
      if (align < type->GetWidth()) {
        Error(loc_,
              "requested alignment is less than minimum alignment of {} for "
              "type '{}'",
              type->GetWidth(), type->ToString());
      }
      dynamic_cast<ObjectExpr*>(ret)->SetAlign(align);
    }
  }

  curr_scope_->InsertNormal(name, ret);

  return MakeAstNode<Declaration>(ret);
}

/*
 * ExtDecl
 */
ExtDecl* Parser::ParseExternalDecl() {
  auto ext_decl{ParseDecl(true)};

  if (ext_decl == nullptr) {
    return nullptr;
  }

  TryParseAsm();
  TryParseAttributeSpec();

  MarkLoc();
  if (Test(Tag::kLeftBrace)) {
    auto stmt{ext_decl->GetStmts()};
    if (std::size(stmt) != 1) {
      Error(loc_, "unexpect left braces");
    }

    return ParseFuncDef(dynamic_cast<Declaration*>(stmt.front()));
  } else {
    Expect(Tag::kSemicolon);
    return ext_decl;
  }
}

FuncDef* Parser::ParseFuncDef(const Declaration* decl) {
  auto ident{decl->GetIdent()};
  if (!ident->GetType()->IsFunctionTy()) {
    Error(decl->GetLoc(), "func def need func type");
  }

  EnterFunc(ident);
  curr_func_def_->SetBody(ParseCompoundStmt(ident->GetType()));
  auto ret{curr_func_def_};
  ExitFunc();

  return ret;
}

/*
 * Expr
 */
Expr* Parser::ParseExpr() {
  // GNU 扩展, 语句表达式
  if (Try(Tag::kLeftParen)) {
    if (Test(Tag::kLeftBrace)) {
      return ParseStmtExpr();
    } else {
      PutBack();
    }
  }

  auto lhs{ParseAssignExpr()};

  MarkLoc();
  while (Try(Tag::kComma)) {
    auto rhs{ParseAssignExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kComma, lhs, rhs);

    MarkLoc();
  }

  return lhs;
}

Expr* Parser::ParseAssignExpr() {
  // 因为有很多是直接调用该函数而不是 ParseExpr
  if (Try(Tag::kLeftParen)) {
    if (Test(Tag::kLeftBrace)) {
      return ParseStmtExpr();
    } else {
      PutBack();
    }
  }

  auto lhs{ParseConditionExpr()};
  Expr* rhs;

  MarkLoc();

  switch (Next().GetTag()) {
    case Tag::kEqual:
      rhs = ParseAssignExpr();
      break;
    case Tag::kStarEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kStar, lhs, rhs);
      break;
    case Tag::kSlashEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kSlash, lhs, rhs);
      break;
    case Tag::kPercentEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kPercent, lhs, rhs);
      break;
    case Tag::kPlusEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kPlus, lhs, rhs);
      break;
    case Tag::kMinusEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kMinus, lhs, rhs);
      break;
    case Tag::kLessLessEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kLessLess, lhs, rhs);
      break;
    case Tag::kGreaterGreaterEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kGreaterGreater, lhs, rhs);
      break;
    case Tag::kAmpEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kAmp, lhs, rhs);
      break;
    case Tag::kCaretEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kCaret, lhs, rhs);
      break;
    case Tag::kPipeEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kPipe, lhs, rhs);
      break;
    default: {
      PutBack();
      return lhs;
    }
  }

  // TODO 代码报错位置不对
  MarkLoc();
  return MakeAstNode<BinaryOpExpr>(Tag::kEqual, lhs, rhs);
}

Expr* Parser::ParseConditionExpr() {
  auto cond{ParseLogicalOrExpr()};

  MarkLoc();

  if (Try(Tag::kQuestion)) {
    // GNU 扩展
    // a ?: b 相当于 a ? a: c
    auto lhs{Test(Tag::kColon) ? cond : ParseExpr()};
    Expect(Tag::kColon);
    auto rhs{ParseConditionExpr()};

    return MakeAstNode<ConditionOpExpr>(cond, lhs, rhs);
  }

  return cond;
}

Expr* Parser::ParseLogicalOrExpr() {
  auto lhs{ParseLogicalAndExpr()};

  MarkLoc();

  while (Try(Tag::kPipePipe)) {
    auto rhs{ParseLogicalAndExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kPipePipe, lhs, rhs);

    MarkLoc();
  }

  return lhs;
}

Expr* Parser::ParseLogicalAndExpr() {
  auto lhs{ParseInclusiveOrExpr()};

  MarkLoc();

  while (Try(Tag::kAmpAmp)) {
    auto rhs{ParseInclusiveOrExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kAmpAmp, lhs, rhs);

    MarkLoc();
  }

  return lhs;
}

Expr* Parser::ParseInclusiveOrExpr() {
  auto lhs{ParseExclusiveOrExpr()};

  MarkLoc();

  while (Try(Tag::kPipe)) {
    auto rhs{ParseExclusiveOrExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kPipe, lhs, rhs);

    MarkLoc();
  }

  return lhs;
}

Expr* Parser::ParseExclusiveOrExpr() {
  auto lhs{ParseAndExpr()};

  MarkLoc();

  while (Try(Tag::kCaret)) {
    auto rhs{ParseAndExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kCaret, lhs, rhs);

    MarkLoc();
  }

  return lhs;
}

Expr* Parser::ParseAndExpr() {
  auto lhs{ParseEqualityExpr()};

  MarkLoc();

  while (Try(Tag::kAmp)) {
    auto rhs{ParseEqualityExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kAmp, lhs, rhs);

    MarkLoc();
  }

  return lhs;
}

Expr* Parser::ParseEqualityExpr() {
  auto lhs{ParseRelationExpr()};

  MarkLoc();

  while (true) {
    if (Try(Tag::kEqualEqual)) {
      auto rhs{ParseRelationExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kEqualEqual, lhs, rhs);
      MarkLoc();
    } else if (Try(Tag::kExclaimEqual)) {
      auto rhs{ParseRelationExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kExclaimEqual, lhs, rhs);
      MarkLoc();
    } else {
      break;
    }
  }

  return lhs;
}

Expr* Parser::ParseRelationExpr() {
  auto lhs{ParseShiftExpr()};

  MarkLoc();

  while (true) {
    if (Try(Tag::kLess)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLess, lhs, rhs);
      MarkLoc();
    } else if (Try(Tag::kGreater)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreater, lhs, rhs);
      MarkLoc();
    } else if (Try(Tag::kLessEqual)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLessEqual, lhs, rhs);
      MarkLoc();
    } else if (Try(Tag::kGreaterEqual)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreaterEqual, lhs, rhs);
      MarkLoc();
    } else {
      break;
    }
  }

  return lhs;
}

Expr* Parser::ParseShiftExpr() {
  auto lhs{ParseAdditiveExpr()};

  MarkLoc();

  while (true) {
    if (Try(Tag::kLessLess)) {
      auto rhs{ParseAdditiveExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLessLess, lhs, rhs);
      MarkLoc();
    } else if (Try(Tag::kGreaterGreater)) {
      auto rhs{ParseAdditiveExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreaterGreater, lhs, rhs);
      MarkLoc();
    } else {
      break;
    }
  }

  return lhs;
}

Expr* Parser::ParseAdditiveExpr() {
  auto lhs{ParseMultiplicativeExpr()};

  MarkLoc();

  while (true) {
    if (Try(Tag::kPlus)) {
      auto rhs{ParseMultiplicativeExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kPlus, lhs, rhs);
      MarkLoc();
    } else if (Try(Tag::kMinus)) {
      auto rhs{ParseMultiplicativeExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kMinus, lhs, rhs);
      MarkLoc();
    } else {
      break;
    }
  }

  return lhs;
}

Expr* Parser::ParseMultiplicativeExpr() {
  auto lhs{ParseCastExpr()};

  MarkLoc();

  while (true) {
    if (Try(Tag::kStar)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kStar, lhs, rhs);
      MarkLoc();
    } else if (Try(Tag::kSlash)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kSlash, lhs, rhs);
      MarkLoc();
    } else if (Try(Tag::kPercent)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kPercent, lhs, rhs);
      MarkLoc();
    } else {
      break;
    }
  }

  return lhs;
}

Expr* Parser::ParseCastExpr() {
  MarkLoc();

  if (Try(Tag::kLeftParen)) {
    if (IsTypeName(Peek())) {
      auto type{ParseTypeName()};
      Expect(Tag::kRightParen);

      // 复合字面量
      if (Test(Tag::kLeftBrace)) {
        return ParsePostfixExprTail(ParseCompoundLiteral(type));
      } else {
        return MakeAstNode<TypeCastExpr>(ParseCastExpr(), type);
      }
    } else {
      PutBack();
      return ParseUnaryExpr();
    }
  } else {
    return ParseUnaryExpr();
  }
}

Expr* Parser::ParseUnaryExpr() {
  switch (Next().GetTag()) {
    // 默认为前缀
    case Tag::kPlusPlus:
      return MakeAstNode<UnaryOpExpr>(Tag::kPlusPlus, ParseUnaryExpr());
    case Tag::kMinusMinus:
      return MakeAstNode<UnaryOpExpr>(Tag::kMinusMinus, ParseUnaryExpr());
    case Tag::kAmp:
      return MakeAstNode<UnaryOpExpr>(Tag::kAmp, ParseCastExpr());
    case Tag::kStar:
      return MakeAstNode<UnaryOpExpr>(Tag::kStar, ParseCastExpr());
    case Tag::kPlus:
      return MakeAstNode<UnaryOpExpr>(Tag::kPlus, ParseCastExpr());
    case Tag::kMinus:
      return MakeAstNode<UnaryOpExpr>(Tag::kMinus, ParseCastExpr());
    case Tag::kTilde:
      return MakeAstNode<UnaryOpExpr>(Tag::kTilde, ParseCastExpr());
    case Tag::kExclaim:
      return MakeAstNode<UnaryOpExpr>(Tag::kExclaim, ParseCastExpr());
    case Tag::kSizeof:
      return ParseSizeof();
    case Tag::kAlignof:
      return ParseAlignof();
    case Tag::kOffsetof:
      return ParseOffsetof();
    case Tag::kRegClass:
      return ParseRegClass();
    default:
      PutBack();
      return ParsePostfixExpr();
  }
}

Expr* Parser::ParseSizeof() {
  QualType type;
  MarkLoc();

  if (Try(Tag::kLeftParen)) {
    if (!IsTypeName(Peek())) {
      auto expr{ParseExpr()};
      type = expr->GetType();
    } else {
      type = ParseTypeName();
    }
    Expect(Tag::kRightParen);
  } else {
    auto expr{ParseUnaryExpr()};
    type = expr->GetType();
  }

  if (!type->IsComplete() && !type->IsVoidTy() && !type->IsFunctionTy()) {
    Error(loc_, "sizeof(incomplete type)");
  }

  return MakeAstNode<ConstantExpr>(
      ArithmeticType::Get(kLong | kUnsigned),
      static_cast<std::uint64_t>(type->GetWidth()));
}

Expr* Parser::ParseAlignof() {
  QualType type;

  Expect(Tag::kLeftParen);

  MarkLoc();
  if (!IsTypeName(Peek())) {
    Error(loc_, "expect type name");
  }

  type = ParseTypeName();
  Expect(Tag::kRightParen);

  return MakeAstNode<ConstantExpr>(type->GetAlign());
}

Expr* Parser::ParsePostfixExpr() {
  auto expr{TryParseCompoundLiteral()};

  if (expr) {
    return ParsePostfixExprTail(expr);
  } else {
    return ParsePostfixExprTail(ParsePrimaryExpr());
  }
}

Expr* Parser::TryParseCompoundLiteral() {
  auto begin{index_};

  if (Try(Tag::kLeftParen) && IsTypeName(Peek())) {
    auto type{ParseTypeName()};

    if (Try(Tag::kRightParen) && Test(Tag::kLeftBrace)) {
      return ParseCompoundLiteral(type);
    }
  }

  index_ = begin;
  return nullptr;
}

Expr* Parser::ParseCompoundLiteral(QualType type) {
  auto linkage{curr_scope_->IsFileScope() ? kInternal : kNone};
  auto obj{MakeAstNode<ObjectExpr>("", type, 0, linkage, true)};
  auto decl{MakeAstNode<Declaration>(obj)};

  decl->AddInits(ParseInitDeclaratorSub(decl->GetIdent()));
  unit_->AddExtDecl(decl);

  return obj;
}

Expr* Parser::ParsePostfixExprTail(Expr* expr) {
  while (true) {
    MarkLoc();

    switch (Next().GetTag()) {
      case Tag::kLeftSquare:
        expr = ParseIndexExpr(expr);
        break;
      case Tag::kLeftParen:
        expr = ParseFuncCallExpr(expr);
        break;
      case Tag::kArrow:
        expr = MakeAstNode<UnaryOpExpr>(Tag::kStar, expr);
        expr = ParseMemberRefExpr(expr);
        break;
      case Tag::kPeriod:
        expr = ParseMemberRefExpr(expr);
        break;
      case Tag::kPlusPlus:
        expr = MakeAstNode<UnaryOpExpr>(Tag::kPostfixPlusPlus, expr);
        break;
      case Tag::kMinusMinus:
        expr = MakeAstNode<UnaryOpExpr>(Tag::kPostfixMinusMinus, expr);
        break;
      default:
        PutBack();
        return expr;
    }
  }
}

Expr* Parser::ParseIndexExpr(Expr* expr) {
  MarkLoc();

  auto rhs{ParseExpr()};
  Expect(Tag::kRightSquare);

  return MakeAstNode<UnaryOpExpr>(
      Tag::kStar, MakeAstNode<BinaryOpExpr>(Tag::kPlus, expr, rhs));
}

Expr* Parser::ParseFuncCallExpr(Expr* expr) {
  MarkLoc();

  std::vector<Expr*> args;
  while (!Try(Tag::kRightParen)) {
    args.push_back(ParseAssignExpr());

    if (!Test(Tag::kRightParen)) {
      Expect(Tag::kComma);
    }
  }

  return MakeAstNode<FuncCallExpr>(expr, args);
}

Expr* Parser::ParseMemberRefExpr(Expr* expr) {
  MarkLoc();

  auto member{Expect(Tag::kIdentifier)};
  auto member_name{member.GetIdentifier()};

  auto type{expr->GetType()};
  if (!type->IsStructOrUnionTy()) {
    Error(loc_, "an struct/union expected: '{}'", type->ToString());
  }

  auto rhs{type->StructGetMember(member_name)};
  if (!rhs) {
    Error(loc_, "'{}' is not a member of '{}'", member_name,
          type->StructGetName());
  }

  return MakeAstNode<BinaryOpExpr>(Tag::kPeriod, expr, rhs);
}

Expr* Parser::ParsePrimaryExpr() {
  MarkLoc();

  if (Peek().IsIdentifier()) {
    auto name{Next().GetIdentifier()};
    auto ident{curr_scope_->FindNormal(name)};

    if (ident) {
      return ident;
    } else {
      Error(loc_, "undefined symbol: {}", name);
    }
  } else if (Peek().IsConstant()) {
    return ParseConstant();
  } else if (Peek().IsStringLiteral()) {
    return ParseStringLiteral();
  } else if (Try(Tag::kLeftParen)) {
    auto expr{ParseExpr()};
    Expect(Tag::kRightParen);
    return expr;
  } else if (Try(Tag::kGeneric)) {
    return ParseGenericSelection();
  } else if (Try(Tag::kFuncName)) {
    return MakeAstNode<StringLiteralExpr>(curr_func_def_->GetName());
  } else {
    Error(loc_, "{} unexpected", Peek().GetStr());
  }
}

Expr* Parser::ParseConstant() {
  if (Peek().IsCharacterConstant()) {
    return ParseCharacter();
  } else if (Peek().IsIntegerConstant()) {
    return ParseInteger();
  } else if (Peek().IsFloatConstant()) {
    return ParseFloat();
  } else {
    assert(false);
    return nullptr;
  }
}

Expr* Parser::ParseCharacter() {
  MarkLoc();

  auto tok{Next()};
  auto [val, encoding]{Scanner{tok.GetStr()}.HandleCharacter()};

  std::uint32_t type_spec{};
  switch (encoding) {
    case Encoding::kNone:
      val = static_cast<char>(val);
      type_spec = kInt;
      break;
    case Encoding::kChar16:
      val = static_cast<char16_t>(val);
      type_spec = kShort | kUnsigned;
      break;
    case Encoding::kChar32:
      val = static_cast<char32_t>(val);
      type_spec = kInt | kUnsigned;
      break;
    case Encoding::kWchar:
      val = static_cast<wchar_t>(val);
      type_spec = kInt | kUnsigned;
      break;
    case Encoding::kUtf8:
      Error(tok, "Can't use u8 here");
    default:
      assert(false);
  }

  return MakeAstNode<ConstantExpr>(ArithmeticType::Get(type_spec),
                                   static_cast<std::uint64_t>(val));
}

Expr* Parser::ParseInteger() {
  MarkLoc();

  auto tok{Next()};
  auto str{tok.GetStr()};
  std::uint64_t val;
  std::size_t end;

  try {
    // GNU 扩展, 也可以有后缀
    if (std::size(str) >= 3 &&
        (str.substr(0, 2) == "0b" || str.substr(0, 2) == "0B")) {
      val = std::stoull(str.substr(2), &end, 2);
      end += 2;
    } else {
      // 当 base 为 0 时，自动检测进制
      val = std::stoull(str, &end, 0);
    }
  } catch (const std::out_of_range& error) {
    Error(tok, "integer out of range");
  }

  auto backup{end};
  std::uint32_t type_spec{};
  for (auto ch{str[end]}; ch != '\0'; ch = str[++end]) {
    if (ch == 'u' || ch == 'U') {
      if (type_spec & kUnsigned) {
        Error(tok, "invalid suffix: {}", str.substr(backup));
      }
      type_spec |= kUnsigned;
    } else if (ch == 'l' || ch == 'L') {
      if ((type_spec & kLong) || (type_spec & kLongLong)) {
        Error(tok, "invalid suffix: {}", str.substr(backup));
      }

      if (str[end + 1] == 'l' || str[end + 1] == 'L') {
        type_spec |= kLongLong;
        ++end;
      } else {
        type_spec |= kLong;
      }
    } else {
      Error(tok, "invalid suffix: {}", str.substr(backup));
    }
  }

  // 十进制
  bool decimal{'1' <= str.front() && str.front() <= '9'};

  if (decimal) {
    switch (type_spec) {
      case 0:
        if ((val > static_cast<std::uint64_t>(
                       std::numeric_limits<std::int32_t>::max()))) {
          type_spec |= kLong;
        } else {
          type_spec |= kInt;
        }
        break;
      case kUnsigned:
        if (val > std::numeric_limits<std::uint32_t>::max()) {
          type_spec |= (kLong | kUnsigned);
        } else {
          type_spec |= (kInt | kUnsigned);
        }
        break;
      default:
        break;
    }
  } else {
    switch (type_spec) {
      case 0:
        if (val > static_cast<std::uint64_t>(
                      std::numeric_limits<std::int64_t>::max())) {
          type_spec |= (kLong | kUnsigned);
        } else if (val > std::numeric_limits<std::uint32_t>::max()) {
          type_spec |= kLong;
        } else if (val > static_cast<std::uint64_t>(
                             std::numeric_limits<std::int32_t>::max())) {
          type_spec |= (kInt | kUnsigned);
        } else {
          type_spec |= kInt;
        }
        break;
      case kUnsigned:
        if (val > std::numeric_limits<std::uint32_t>::max()) {
          type_spec |= (kLong | kUnsigned);
        } else {
          type_spec |= (kInt | kUnsigned);
        }
        break;
      case kLong:
        if (val > static_cast<std::uint64_t>(
                      std::numeric_limits<std::int64_t>::max())) {
          type_spec |= (kLong | kUnsigned);
        } else {
          type_spec |= kLong;
        }
        break;
      case kLongLong:
        if (val > static_cast<std::uint64_t>(
                      std::numeric_limits<std::int64_t>::max())) {
          type_spec |= (kLongLong | kUnsigned);
        } else {
          type_spec |= kLongLong;
        }
        break;
      default:
        break;
    }
  }

  return MakeAstNode<ConstantExpr>(ArithmeticType::Get(type_spec), val);
}

Expr* Parser::ParseFloat() {
  MarkLoc();

  auto tok{Next()};
  auto str{tok.GetStr()};
  long double val;
  std::size_t end;

  try {
    val = std::stold(str, &end);
  } catch (const std::out_of_range& err) {
    std::istringstream iss{str};
    iss >> val;
    end = iss.tellg();

    // 当值过小时是可以的
    if (std::abs(val) > 1.0) {
      Error(tok, "float point out of range");
    }
  }

  auto backup{end};
  std::uint32_t type_spec{kDouble};
  if (str[end] == 'f' || str[end] == 'F') {
    type_spec = kFloat;
    ++end;
  } else if (str[end] == 'l' || str[end] == 'L') {
    type_spec = kLong | kDouble;
    ++end;
  }

  if (str[end] != '\0') {
    Error(tok, "invalid suffix:{}", str.substr(backup));
  }

  return MakeAstNode<ConstantExpr>(ArithmeticType::Get(type_spec), val);
}

StringLiteralExpr* Parser::ParseStringLiteral() {
  MarkLoc();

  // 如果一个没有指定编码而另一个指定了那么可以连接
  // 两个都指定了不能连接
  auto tok{Expect(Tag::kStringLiteral)};
  auto [str, encoding]{Scanner{tok.GetStr()}.HandleStringLiteral()};
  ConvertStringLiteral(str, encoding);

  while (Test(Tag::kStringLiteral)) {
    tok = Next();
    auto [next_str, next_encoding]{Scanner{tok.GetStr()}.HandleStringLiteral()};
    ConvertStringLiteral(next_str, next_encoding);

    if (encoding == Encoding::kNone && next_encoding != Encoding::kNone) {
      ConvertStringLiteral(str, next_encoding);
      encoding = next_encoding;
    } else if (encoding != Encoding::kNone &&
               next_encoding == Encoding::kNone) {
      ConvertStringLiteral(next_str, encoding);
      next_encoding = encoding;
    }

    if (encoding != next_encoding) {
      Error(loc_, "cannot concat literal with different encodings");
    }

    str += next_str;

    MarkLoc();
  }

  std::uint32_t type_spec{};

  switch (encoding) {
    case Encoding::kUtf8:
    case Encoding::kNone:
      type_spec = kChar;
      str.append(1, '\0');
      break;
    case Encoding::kChar16:
      type_spec = kShort | kUnsigned;
      str.append(2, '\0');
      break;
    case Encoding::kChar32:
    case Encoding::kWchar:
      type_spec = kInt | kUnsigned;
      str.append(4, '\0');
      break;
    default:
      assert(false);
  }

  return MakeAstNode<StringLiteralExpr>(ArithmeticType::Get(type_spec), str);
}

Expr* Parser::ParseGenericSelection() {
  Expect(Tag::kLeftParen);
  auto control_expr{ParseAssignExpr()};
  Expect(Tag::kComma);

  Expr* ret{nullptr};
  Expr* default_expr{nullptr};

  while (true) {
    MarkLoc();

    if (Try(Tag::kDefault)) {
      if (default_expr) {
        Error(loc_, "duplicate default generic association");
      }

      Expect(Tag::kColon);
      default_expr = ParseAssignExpr();
    } else {
      auto type{ParseTypeName()};

      if (type->Compatible(control_expr->GetType())) {
        if (ret) {
          Error(loc_,
                "more than one generic association are compatible with control "
                "expression");
        }

        Expect(Tag::kColon);
        ret = ParseAssignExpr();
      } else {
        Expect(Tag::kColon);
        ParseAssignExpr();
      }
    }

    if (!Try(Tag::kComma)) {
      Expect(Tag::kRightParen);
      break;
    }
  }

  if (!ret && !default_expr) {
    Error(Peek(), "no compatible generic association");
  }

  return ret ? ret : default_expr;
}

Expr* Parser::ParseConstantExpr() { return ParseConditionExpr(); }

/*
 * Stmt
 */
Stmt* Parser::ParseStmt() {
  TryParseAttributeSpec();

  switch (Peek().GetTag()) {
    case Tag::kIdentifier: {
      Next();
      if (Peek().TagIs(Tag::kColon)) {
        PutBack();
        return ParseLabelStmt();
      } else {
        PutBack();
        return ParseExprStmt();
      }
    }
    case Tag::kCase:
      return ParseCaseStmt();
    case Tag::kDefault:
      return ParseDefaultStmt();
    case Tag::kLeftBrace:
      return ParseCompoundStmt();
    case Tag::kIf:
      return ParseIfStmt();
    case Tag::kSwitch:
      return ParseSwitchStmt();
    case Tag::kWhile:
      return ParseWhileStmt();
    case Tag::kDo:
      return ParseDoWhileStmt();
    case Tag::kFor:
      return ParseForStmt();
    case Tag::kGoto:
      return ParseGotoStmt();
    case Tag::kContinue:
      return ParseContinueStmt();
    case Tag::kBreak:
      return ParseBreakStmt();
    case Tag::kReturn:
      return ParseReturnStmt();
    default:
      return ParseExprStmt();
  }
}

Stmt* Parser::ParseLabelStmt() {
  auto tok{Expect(Tag::kIdentifier)};
  Expect(Tag::kColon);

  TryParseAttributeSpec();

  return MakeAstNode<LabelStmt>(MakeAstNode<IdentifierExpr>(
      tok.GetIdentifier(), VoidType::Get(), kNone, false));
}

Stmt* Parser::ParseCaseStmt() {
  Expect(Tag::kCase);

  MarkLoc();
  auto expr{ParseExpr()};
  if (!expr->GetType()->IsIntegerTy()) {
    Error(expr, "expect integer");
  }
  auto val{CalcExpr<std::uint64_t>{}.Calc(expr)};

  if (val >
      static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
    Error(loc_, "case range exceed range of int");
  }

  // GNU 扩展
  if (Try(Tag::kEllipsis)) {
    MarkLoc();

    auto expr2{ParseExpr()};
    if (!expr2->GetType()->IsIntegerTy()) {
      Error(expr2, "expect integer");
    }
    auto val2{CalcExpr<std::uint64_t>{}.Calc(expr)};
    if (val >
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
      Error(loc_, "case range exceed range of int");
    }

    Expect(Tag::kColon);
    return MakeAstNode<CaseStmt>(val, val2, ParseStmt());
  } else {
    Expect(Tag::kColon);
    return MakeAstNode<CaseStmt>(val, ParseStmt());
  }
}

Stmt* Parser::ParseDefaultStmt() {
  Expect(Tag::kDefault);
  Expect(Tag::kColon);

  return MakeAstNode<DefaultStmt>(ParseStmt());
}

CompoundStmt* Parser::ParseCompoundStmt(Type* func_type) {
  Expect(Tag::kLeftBrace);

  EnterBlock(func_type);

  std::vector<Stmt*> stmts;
  while (!Try(Tag::kRightBrace)) {
    if (IsDecl(Peek())) {
      if (auto decl{ParseDecl()}; decl) {
        stmts.push_back(decl);
      }
    } else {
      stmts.push_back(ParseStmt());
    }
  }

  auto scope{curr_scope_};
  ExitBlock();

  return MakeAstNode<CompoundStmt>(stmts, scope);
}

Stmt* Parser::ParseExprStmt() {
  if (Try(Tag::kSemicolon)) {
    return MakeAstNode<ExprStmt>();
  } else {
    auto ret{MakeAstNode<ExprStmt>(ParseExpr())};
    Expect(Tag::kSemicolon);
    return ret;
  }
}

Stmt* Parser::ParseIfStmt() {
  MarkLoc();

  Expect(Tag::kIf);

  Expect(Tag::kLeftParen);
  auto cond{ParseExpr()};
  Expect(Tag::kRightParen);

  auto then_block{ParseStmt()};
  if (Try(Tag::kElse)) {
    return MakeAstNode<IfStmt>(cond, then_block, ParseStmt());
  } else {
    return MakeAstNode<IfStmt>(cond, then_block);
  }
}

Stmt* Parser::ParseSwitchStmt() {
  Expect(Tag::kSwitch);

  Expect(Tag::kLeftParen);
  auto cond{ParseExpr()};
  Expect(Tag::kRightParen);

  return MakeAstNode<SwitchStmt>(cond, ParseStmt());
}

Stmt* Parser::ParseWhileStmt() {
  MarkLoc();

  Expect(Tag::kWhile);

  Expect(Tag::kLeftParen);
  auto cond{ParseExpr()};
  Expect(Tag::kRightParen);

  return MakeAstNode<WhileStmt>(cond, ParseStmt());
}

Stmt* Parser::ParseDoWhileStmt() {
  MarkLoc();

  Expect(Tag::kDo);

  auto stmt{ParseStmt()};

  Expect(Tag::kWhile);
  Expect(Tag::kLeftParen);
  auto cond{ParseExpr()};
  Expect(Tag::kRightParen);
  Expect(Tag::kSemicolon);

  return MakeAstNode<DoWhileStmt>(cond, stmt);
}

Stmt* Parser::ParseForStmt() {
  MarkLoc();

  Expect(Tag::kFor);
  Expect(Tag::kLeftParen);

  Expr *init{}, *cond{}, *inc{};
  Stmt* block{};
  Stmt* decl{};

  EnterBlock();
  if (IsDecl(Peek())) {
    decl = ParseDecl(false);
  } else if (!Try(Tag::kSemicolon)) {
    init = ParseExpr();
    Expect(Tag::kSemicolon);
  }

  if (!Try(Tag::kSemicolon)) {
    cond = ParseExpr();
    Expect(Tag::kSemicolon);
  }

  if (!Try(Tag::kRightParen)) {
    inc = ParseExpr();
    Expect(Tag::kRightParen);
  }

  block = ParseStmt();
  ExitBlock();

  return MakeAstNode<ForStmt>(init, cond, inc, block, decl);
}

Stmt* Parser::ParseGotoStmt() {
  MarkLoc();

  Expect(Tag::kGoto);
  auto tok{Expect(Tag::kIdentifier)};
  Expect(Tag::kSemicolon);

  return MakeAstNode<GotoStmt>(MakeAstNode<IdentifierExpr>(
      tok.GetIdentifier(), VoidType::Get(), kNone, false));
}

Stmt* Parser::ParseContinueStmt() {
  MarkLoc();

  Expect(Tag::kContinue);
  Expect(Tag::kSemicolon);

  return MakeAstNode<ContinueStmt>();
}

Stmt* Parser::ParseBreakStmt() {
  MarkLoc();

  Expect(Tag::kBreak);
  Expect(Tag::kSemicolon);

  return MakeAstNode<BreakStmt>();
}

Stmt* Parser::ParseReturnStmt() {
  MarkLoc();

  Expect(Tag::kReturn);

  if (Try(Tag::kSemicolon)) {
    return MakeAstNode<ReturnStmt>();
  } else {
    auto expr{ParseExpr()};
    Expect(Tag::kSemicolon);

    expr = Expr::MayCastTo(expr,
                           curr_func_def_->GetFuncType()->FuncGetReturnType());
    return MakeAstNode<ReturnStmt>(expr);
  }
}

/*
 * Decl
 */
CompoundStmt* Parser::ParseDecl(bool maybe_func_def) {
  if (Try(Tag::kStaticAssert)) {
    ParseStaticAssertDecl();
    return nullptr;
  } else {
    std::uint32_t storage_class_spec{}, func_spec{};
    std::int32_t align{};
    auto base_type{ParseDeclSpec(&storage_class_spec, &func_spec, &align)};

    if (Try(Tag::kSemicolon)) {
      return nullptr;
    } else {
      if (maybe_func_def) {
        return ParseInitDeclaratorList(base_type, storage_class_spec, func_spec,
                                       align);
      } else {
        auto ret{ParseInitDeclaratorList(base_type, storage_class_spec,
                                         func_spec, align)};
        Expect(Tag::kSemicolon);
        return ret;
      }
    }
  }
}

void Parser::ParseStaticAssertDecl() {
  Expect(Tag::kLeftParen);
  auto expr{ParseConstantExpr()};
  Expect(Tag::kComma);

  // TODO 不处理转义序列
  auto msg{ParseStringLiteral()->GetVal()};
  Expect(Tag::kRightParen);
  Expect(Tag::kSemicolon);

  if (!CalcExpr<std::int32_t>{}.Calc(expr)) {
    Error(expr, "static_assert failed \"{}\"", msg);
  }
}

/*
 * Decl Spec
 */
QualType Parser::ParseDeclSpec(std::uint32_t* storage_class_spec,
                               std::uint32_t* func_spec, std::int32_t* align) {
#define CheckAndSetStorageClassSpec(spec)                       \
  if (*storage_class_spec != 0) {                               \
    Error(tok, "duplicated storage class specifier");           \
  } else if (!storage_class_spec) {                             \
    Error(tok, "storage class specifier are not allowed here"); \
  }                                                             \
  *storage_class_spec |= spec;

#define CheckAndSetFuncSpec(spec)                                       \
  if (*func_spec & spec) {                                              \
    Warning(tok, "duplicate function specifier declaration specifier"); \
  } else if (!func_spec) {                                              \
    Error(tok, "function specifiers are not allowed here");             \
  }                                                                     \
  *func_spec |= spec;

#define ERROR Error(tok, "two or more data types in declaration specifiers");

#define TypeofCheck                                               \
  if (has_typeof) {                                               \
    Error(tok, "It is not allowed to use type specifiers here."); \
  }

  std::uint32_t type_spec{}, type_qual{};
  bool has_typeof{false};

  Token tok;
  QualType type;

  while (true) {
    tok = Next();

    switch (tok.GetTag()) {
      // GNU 扩展
      case Tag::kExtension:
        break;
      case Tag::kTypeof:
        if (type_spec != 0) {
          Error(tok, "It is not allowed to use typeof here.");
        }
        type = ParseTypeof();
        has_typeof = true;
        break;

        // Storage Class Specifier, 至多有一个
      case Tag::kTypedef:
        CheckAndSetStorageClassSpec(kTypedef) break;
      case Tag::kExtern:
        CheckAndSetStorageClassSpec(kExtern) break;
      case Tag::kStatic:
        CheckAndSetStorageClassSpec(kStatic) break;
      case Tag::kAuto:
        CheckAndSetStorageClassSpec(kAuto) break;
      case Tag::kRegister:
        CheckAndSetStorageClassSpec(kRegister) break;
      case Tag::kThreadLocal:
        Error(tok, "Does not support _Thread_local");

        // Type specifier
      case Tag::kVoid:
        if (type_spec) {
          ERROR
        }
        TypeofCheck type_spec |= kVoid;
        break;
      case Tag::kChar:
        if (type_spec & ~kCompChar) {
          ERROR
        }
        TypeofCheck type_spec |= kChar;
        break;
      case Tag::kShort:
        if (type_spec & ~kCompShort) {
          ERROR
        }
        TypeofCheck type_spec |= kShort;
        break;
      case Tag::kInt:
        if (type_spec & ~kCompInt) {
          ERROR
        }
        TypeofCheck type_spec |= kInt;
        break;
      case Tag::kLong:
        if (type_spec & ~kCompLong) {
          ERROR
        }
        TypeofCheck if (type_spec & kLong) {
          type_spec &= ~kLong;
          type_spec |= kLongLong;
        }
        else {
          type_spec |= kLong;
        }
        break;
      case Tag::kFloat:
        if (type_spec) {
          ERROR
        }
        TypeofCheck type_spec |= kFloat;
        break;
      case Tag::kDouble:
        if (type_spec & ~kCompDouble) {
          ERROR
        }
        TypeofCheck type_spec |= kDouble;
        break;
      case Tag::kSigned:
        if (type_spec & ~kCompSigned) {
          ERROR
        }
        TypeofCheck type_spec |= kSigned;
        break;
      case Tag::kUnsigned:
        if (type_spec & ~kCompUnsigned) {
          ERROR
        }
        TypeofCheck type_spec |= kUnsigned;
        break;
      case Tag::kBool:
        if (type_spec) {
          ERROR
        }
        TypeofCheck type_spec |= kBool;
        break;
      case Tag::kStruct:
      case Tag::kUnion:
        if (type_spec) {
          ERROR
        }
        TypeofCheck type = ParseStructUnionSpec(tok.GetTag() == Tag::kStruct);
        type_spec |= kStructUnionSpec;
        break;
      case Tag::kEnum:
        if (type_spec) {
          ERROR
        }
        TypeofCheck type = ParseEnumSpec();
        type_spec |= kEnumSpec;
        break;
      case Tag::kComplex:
        TypeofCheck Error(tok, "Does not support _Complex");
      case Tag::kAtomic:
        TypeofCheck Error(tok, "Does not support _Atomic");

        // Type qualifier
      case Tag::kConst:
        type_qual |= kConst;
        break;
      case Tag::kRestrict:
        type_qual |= kRestrict;
        break;
      case Tag::kVolatile:
        type_qual |= kVolatile;
        break;

        // Function specifier
      case Tag::kInline:
        CheckAndSetFuncSpec(kInline) break;
      case Tag::kNoreturn:
        CheckAndSetFuncSpec(kNoreturn) break;

      case Tag::kAlignas:
        if (!align) {
          Error(tok, "_Alignas are not allowed here");
        }
        *align = std::max(ParseAlignas(), *align);
        break;

      default: {
        if (type_spec == 0 && IsTypeName(tok)) {
          auto ident{curr_scope_->FindNormal(tok.GetIdentifier())};
          type = ident->GetQualType();
          type_spec |= kTypedefName;
        } else {
          goto finish;
        }
      }
    }
  }

finish:
  PutBack();

  TryParseAttributeSpec();

  switch (type_spec) {
    case 0:
      if (!has_typeof) {
        Error(tok, "type specifier missing: {}", tok.GetStr());
      }
      break;
    case kVoid:
      type = VoidType::Get();
      break;
    case kStructUnionSpec:
    case kEnumSpec:
    case kTypedefName:
      break;
    default:
      type = ArithmeticType::Get(type_spec);
  }

  return QualType{type.GetType(), type.GetTypeQual() | type_qual};

#undef CheckAndSetStorageClassSpec
#undef CheckAndSetFuncSpec
#undef ERROR
#undef TypeofCheck
}

Type* Parser::ParseStructUnionSpec(bool is_struct) {
  TryParseAttributeSpec();

  auto tok{Peek()};
  std::string tag_name;

  if (Try(Tag::kIdentifier)) {
    tag_name = tok.GetIdentifier();
    // 定义
    if (Try(Tag::kLeftBrace)) {
      auto tag{curr_scope_->FindTagInCurrScope(tag_name)};
      // 无前向声明
      if (!tag) {
        auto type{StructType::Get(is_struct, tag_name, curr_scope_)};
        auto ident{MakeAstNode<IdentifierExpr>(tag_name, type, kNone, true)};
        curr_scope_->InsertTag(tag_name, ident);

        ParseStructDeclList(type);
        Expect(Tag::kRightBrace);
        return type;
      } else {
        if (tag->GetType()->IsComplete()) {
          Error(tok, "redefinition struct or union :{}", tag_name);
        } else {
          ParseStructDeclList(dynamic_cast<StructType*>(tag->GetType()));

          Expect(Tag::kRightBrace);
          return tag->GetType();
        }
      }
    } else {
      // 可能是前向声明或普通的声明
      auto tag{curr_scope_->FindTag(tag_name)};

      if (tag) {
        return tag->GetType();
      } else {
        auto type{StructType::Get(is_struct, tag_name, curr_scope_)};
        auto ident{MakeAstNode<IdentifierExpr>(tag_name, type, kNone, true)};
        curr_scope_->InsertTag(tag_name, ident);
        return type;
      }
    }
  } else {
    // 无标识符只能是定义
    Expect(Tag::kLeftBrace);

    auto type{StructType::Get(is_struct, "", curr_scope_)};
    ParseStructDeclList(type);

    Expect(Tag::kRightBrace);
    return type;
  }
}

void Parser::ParseStructDeclList(StructType* type) {
  assert(!type->IsComplete());

  auto scope_backup{curr_scope_};
  curr_scope_ = type->GetScope();

  while (!Test(Tag::kRightBrace)) {
    if (Try(Tag::kStaticAssert)) {
      ParseStaticAssertDecl();
    } else {
      std::int32_t align{};
      auto base_type{ParseDeclSpec(nullptr, nullptr, &align)};

      do {
        Token tok;
        auto copy{base_type};
        ParseDeclarator(tok, copy);

        TryParseAttributeSpec();

        // 位域
        if (Try(Tag::kColon)) {
          Error(Peek(), "Bit field not supported");
        }

        // struct A {
        //  int a;
        //  struct {
        //    int c;
        //  };
        //};
        if (std::empty(tok.GetStr())) {
          // 此时该 struct / union 不能有名字
          if (copy->IsStructOrUnionTy() && !copy->StructOrUnionHasName()) {
            auto anonymous{MakeAstNode<ObjectExpr>("", copy, 0, kNone, true)};
            type->MergeAnonymous(anonymous);
            continue;
          } else {
            Error(Peek(), "declaration does not declare anything");
          }
        } else {
          auto name{tok.GetIdentifier()};

          if (type->GetMember(name)) {
            Error(Peek(), "duplicate member:{}", name);
          } else if (copy->IsArrayTy() && !copy->IsComplete()) {
            // 可能是柔性数组
            if (type->IsStruct() && std::size(type->GetMembers()) > 0) {
              auto member{MakeAstNode<ObjectExpr>(name, copy)};
              type->AddMember(member);
              // 必须是最后一个成员
              Expect(Tag::kSemicolon);
              Expect(Tag::kRightBrace);

              goto finalize;
            } else {
              Error(Peek(), "field '{}' has incomplete type", name);
            }
          } else if (copy->IsFunctionTy()) {
            Error(Peek(), "field '{}' declared as a function", name);
          } else {
            auto member{MakeAstNode<ObjectExpr>(name, copy)};
            type->AddMember(member);
          }
        }
      } while (Try(Tag::kComma));

      Expect(Tag::kSemicolon);
    }
  }

finalize:
  TryParseAttributeSpec();

  type->SetComplete(true);

  // struct / union 中的 tag 的作用域与该 struct / union 所在的作用域相同
  for (const auto& [name, tag] : curr_scope_->AllTagInCurrScope()) {
    if (scope_backup->FindTagInCurrScope(name)) {
      Error(tag->GetLoc(), "redefinition of tag {}", tag->GetName());
    } else {
      scope_backup->InsertTag(name, tag);
    }
  }

  curr_scope_ = scope_backup;
}

Type* Parser::ParseEnumSpec() {
  TryParseAttributeSpec();

  std::string tag_name;
  auto tok{Peek()};

  if (Try(Tag::kIdentifier)) {
    tag_name = tok.GetIdentifier();
    // 定义
    if (Try(Tag::kLeftBrace)) {
      auto tag{curr_scope_->FindTagInCurrScope(tag_name)};

      if (!tag) {
        auto type{ArithmeticType::Get(32)};
        auto ident{MakeAstNode<IdentifierExpr>(tag_name, type, kNone, true)};
        curr_scope_->InsertTag(tag_name, ident);
        ParseEnumerator();

        Expect(Tag::kRightBrace);
        return type;
      } else {
        // 不允许前向声明，如果当前作用域中有 tag 则就是重定义
        Error(tok, "redefinition of enumeration tag: {}", tag_name);
      }
    } else {
      // 只能是普通声明
      auto tag{curr_scope_->FindTag(tag_name)};
      if (tag) {
        return tag->GetType();
      } else {
        Error(tok, "unknown enumeration: {}", tag_name);
      }
    }
  } else {
    Expect(Tag::kLeftBrace);
    ParseEnumerator();
    Expect(Tag::kRightBrace);
    return ArithmeticType::Get(32);
  }
}

void Parser::ParseEnumerator() {
  std::int32_t val{};

  do {
    auto tok{Expect(Tag::kIdentifier)};
    TryParseAttributeSpec();

    auto name{tok.GetIdentifier()};
    auto ident{curr_scope_->FindNormalInCurrScope(name)};

    if (ident) {
      Error(tok, "redefinition of enumerator '{}'", name);
    }

    if (Try(Tag::kEqual)) {
      auto expr{ParseConstantExpr()};
      val = CalcExpr<std::int32_t>{}.Calc(expr);
    }

    auto enumer{MakeAstNode<EnumeratorExpr>(tok.GetIdentifier(), val)};
    ++val;
    curr_scope_->InsertNormal(name, enumer);

    Try(Tag::kComma);
  } while (!Test(Tag::kRightBrace));
}

std::int32_t Parser::ParseAlignas() {
  Expect(Tag::kLeftParen);

  std::int32_t align{};
  auto tok{Peek()};

  if (IsTypeName(tok)) {
    auto type{ParseTypeName()};
    align = type->GetAlign();
  } else {
    auto expr{ParseConstantExpr()};
    align = CalcExpr<std::int32_t>{}.Calc(expr);
  }

  Expect(Tag::kRightParen);

  if (align < 0 || ((align - 1) & align)) {
    Error(tok, "requested alignment is not a power of 2");
  }

  return align;
}

/*
 * Declarator
 */
CompoundStmt* Parser::ParseInitDeclaratorList(QualType& base_type,
                                              std::uint32_t storage_class_spec,
                                              std::uint32_t func_spec,
                                              std::int32_t align) {
  auto stmts{MakeAstNode<CompoundStmt>()};

  do {
    auto copy{base_type};
    stmts->AddStmt(
        ParseInitDeclarator(copy, storage_class_spec, func_spec, align));
  } while (Try(Tag::kComma));

  return stmts;
}

Declaration* Parser::ParseInitDeclarator(QualType& base_type,
                                         std::uint32_t storage_class_spec,
                                         std::uint32_t func_spec,
                                         std::int32_t align) {
  Token tok;
  ParseDeclarator(tok, base_type);

  if (std::empty(tok.GetStr())) {
    Error(tok, "expect identifier");
  }

  auto decl{MakeDeclaration(tok.GetIdentifier(), base_type, storage_class_spec,
                            func_spec, align)};

  if (decl && Try(Tag::kEqual) && decl->IsObj()) {
    decl->AddInits(ParseInitDeclaratorSub(decl->GetIdent()));
  }

  return decl;
}

void Parser::ParseDeclarator(Token& tok, QualType& base_type) {
  ParsePointer(base_type);
  ParseDirectDeclarator(tok, base_type);
}

void Parser::ParsePointer(QualType& type) {
  while (Try(Tag::kStar)) {
    type = QualType{PointerType::Get(type), ParseTypeQualList()};
  }
}

std::uint32_t Parser::ParseTypeQualList() {
  MarkLoc();

  std::uint32_t type_qual{};

  while (true) {
    if (Try(Tag::kConst)) {
      type_qual |= kConst;
    } else if (Try(Tag::kRestrict)) {
      type_qual |= kRestrict;
    } else if (Try(Tag::kVolatile)) {
      Error(loc_, "Does not support volatile");
    } else if (Try(Tag::kAtomic)) {
      Error(loc_, "Does not support _Atomic");
    } else {
      break;
    }
  }

  return type_qual;
}

void Parser::ParseDirectDeclarator(Token& tok, QualType& base_type) {
  if (Test(Tag::kIdentifier)) {
    tok = Next();
    ParseDirectDeclaratorTail(base_type);
  } else if (Try(Tag::kLeftParen)) {
    auto begin{index_};
    auto temp{QualType{ArithmeticType::Get(kInt)}};
    // 此时的 base_type 不一定是正确的, 先跳过括号中的内容
    ParseDeclarator(tok, temp);
    Expect(Tag::kRightParen);

    ParseDirectDeclaratorTail(base_type);
    auto end{index_};

    index_ = begin;
    ParseDeclarator(tok, base_type);
    Expect(Tag::kRightParen);
    index_ = end;
  } else {
    ParseDirectDeclaratorTail(base_type);
  }
}

void Parser::ParseDirectDeclaratorTail(QualType& base_type) {
  if (Try(Tag::kLeftSquare)) {
    if (base_type->IsFunctionTy()) {
      Error(Peek(), "the element of array cannot be a function");
    }

    auto len{ParseArrayLength()};
    Expect(Tag::kRightSquare);

    ParseDirectDeclaratorTail(base_type);

    if (!base_type->IsComplete()) {
      Error(Peek(), "has incomplete element type");
    }

    base_type = ArrayType::Get(base_type, len);
  } else if (Try(Tag::kLeftParen)) {
    if (base_type->IsFunctionTy()) {
      Error(Peek(), "the return value of function cannot be function");
    } else if (base_type->IsArrayTy()) {
      Error(Peek(), "the return value of function cannot be array");
    }

    EnterProto();
    auto [params, var_args]{ParseParamTypeList()};
    ExitProto();

    Expect(Tag::kRightParen);

    ParseDirectDeclaratorTail(base_type);

    base_type = FunctionType::Get(base_type, params, var_args);
  }
}

std::size_t Parser::ParseArrayLength() {
  if (Test(Tag::kRightSquare)) {
    return 0;
  }

  auto expr{ParseAssignExpr()};

  if (!expr->GetQualType()->IsIntegerTy()) {
    Error(expr, "The array size must be an integer: '{}'",
          expr->GetType()->ToString());
  }

  // 不支持变长数组
  auto len{CalcExpr<std::int32_t>{}.Calc(expr)};

  if (len < 0) {
    Error(expr, "Array size must be greater than 0: '{}'", len);
  }

  return len;
}

std::pair<std::vector<ObjectExpr*>, bool> Parser::ParseParamTypeList() {
  if (Test(Tag::kRightParen)) {
    //    Warning(
    //        Peek(),
    //        "The parameter list is not allowed to be empty, you should use
    //        void");
    return {{}, false};
  }

  auto param{ParseParamDecl()};
  if (param->GetType()->IsVoidTy()) {
    return {{}, false};
  }

  std::vector<ObjectExpr*> params;
  params.push_back(param);

  while (Try(Tag::kComma)) {
    MarkLoc();

    if (Try(Tag::kEllipsis)) {
      return {params, true};
    }

    param = ParseParamDecl();
    if (param->GetType()->IsVoidTy()) {
      Error(loc_, "'void' must be the first and only parameter if specified");
    }
    params.push_back(param);
  }

  return {params, false};
}

// declaration-specifiers declarator
// declaration-specifiers abstract-declarator（此时不能是函数定义）
ObjectExpr* Parser::ParseParamDecl() {
  auto base_type{ParseDeclSpec(nullptr, nullptr, nullptr)};

  Token tok;
  ParseDeclarator(tok, base_type);

  base_type = Type::MayCast(base_type);

  if (std::empty(tok.GetStr())) {
    return MakeAstNode<ObjectExpr>("", base_type, 0, kNone, true);
  }

  auto decl{MakeDeclaration(tok.GetIdentifier(), base_type, 0, 0, 0)};

  return dynamic_cast<ObjectExpr*>(decl->GetIdent());
}

/*
 * Type Name
 */
QualType Parser::ParseTypeName() {
  auto base_type{ParseDeclSpec(nullptr, nullptr, nullptr)};
  ParseAbstractDeclarator(base_type);
  return base_type;
}

void Parser::ParseAbstractDeclarator(QualType& type) {
  ParsePointer(type);
  ParseDirectAbstractDeclarator(type);
}

void Parser::ParseDirectAbstractDeclarator(QualType& type) {
  Token tok;
  ParseDirectDeclarator(tok, type);

  if (!std::empty(tok.GetStr())) {
    Error(tok, "unexpected identifier '{}'", tok.GetStr());
  }
}

/*
 * Init
 */
Initializers Parser::ParseInitDeclaratorSub(IdentifierExpr* ident) {
  if (!curr_scope_->IsFileScope() && ident->GetLinkage() == kExternal) {
    Error(ident->GetLoc(), "{} has both 'extern' and initializer",
          ident->GetName());
  }

  if (!ident->GetType()->IsComplete() && !ident->GetType()->IsArrayTy()) {
    Error(ident->GetLoc(), "variable '{}' has initializer but incomplete type",
          ident->GetName());
  }

  Initializers inits;
  ParseInitializer(inits, ident->GetType(), 0, false, true);
  return inits;
}

// initializer:
//  assignment-expression
//  { initializer-list }
//  { initializer-list , }
void Parser::ParseInitializer(Initializers& inits, QualType type,
                              std::int32_t offset, bool designated,
                              bool force_brace) {
  // 比如解析 {[2]=1}
  if (designated && !Test(Tag::kPeriod) && !Test(Tag::kLeftSquare)) {
    Expect(Tag::kEqual);
  }

  if (type->IsArrayTy()) {
    // int a[2] = 1;
    // 不能直接 Expect , 如果有 '{' , 只能由 ParseArrayInitializer 来处理
    if (force_brace && !Test(Tag::kLeftBrace) && !Test(Tag::kStringLiteral)) {
      Expect(Tag::kLeftBrace);
    } else if (!ParseLiteralInitializer(inits, type.GetType(), offset)) {
      ParseArrayInitializer(inits, type.GetType(), offset, designated);
      type->SetComplete(true);
    } else {
      return;
    }
  } else if (type->IsStructOrUnionTy()) {
    if (!Test(Tag::kPeriod) && !Test(Tag::kLeftBrace)) {
      // struct A a = {...};
      // struct A b = a;
      // 或者是
      // struct {
      //    struct {
      //      int a;
      //      int b;
      //    } x;
      //    struct {
      //      char c[8];
      //    } y;
      //  } v = {
      //      1,
      //      2,
      //  };
      auto begin{index_};
      auto expr{ParseAssignExpr()};
      if (type->Compatible(expr->GetType())) {
        inits.AddInit({type.GetType(), offset, expr});
        return;
      } else {
        index_ = begin;
      }
    }

    ParseStructInitializer(inits, type.GetType(), offset, designated);
  } else {
    // 标量类型
    // int a={10}; / int a={10,}; 都是合法的
    auto has_brace{Try(Tag::kLeftBrace)};
    auto expr{ParseAssignExpr()};

    if (has_brace) {
      Try(Tag::kComma);
      Expect(Tag::kRightBrace);
    }

    inits.AddInit({type.GetType(), offset, expr});
  }
}

void Parser::ParseArrayInitializer(Initializers& inits, Type* type,
                                   std::int32_t offset,
                                   std::int32_t designated) {
  std::int32_t index{};
  auto width{type->ArrayGetElementType()->GetWidth()};
  auto has_brace{Try(Tag::kLeftBrace)};

  while (true) {
    if (Test(Tag::kRightBrace)) {
      if (has_brace) {
        Next();
      }
      return;
    }

    // TODO ???
    if (!designated && !has_brace &&
        (Test(Tag::kPeriod) || Test(Tag::kLeftSquare))) {
      // put ',' back
      PutBack();
      return;
    }

    if ((designated = Try(Tag::kLeftSquare))) {
      auto expr{ParseAssignExpr()};
      if (!expr->GetType()->IsIntegerTy()) {
        Error(expr, "expect integer type");
      }

      index = CalcExpr<std::int32_t>{}.Calc(expr);
      Expect(Tag::kRightSquare);

      if (index < 0 ||
          (type->IsComplete() &&
           static_cast<std::size_t>(index) >= type->ArrayGetNumElements())) {
        Error(expr, "array designator index {} exceeds array bounds", index);
      }
    }

    ParseInitializer(inits, type->ArrayGetElementType(), offset + index * width,
                     designated, false);
    designated = false;
    ++index;

    // int a[] = {1, 2, [5] = 3}; 这种也是合法的
    if (!type->IsComplete()) {
      type->ArraySetNumElements(std::max(static_cast<std::size_t>(index),
                                         type->ArrayGetNumElements()));
    }

    if (!Try(Tag::kComma)) {
      if (has_brace) {
        Expect(Tag::kRightBrace);
      }
      return;
    }
  }
}

// TODO 对剩余元素值初始化
bool Parser::ParseLiteralInitializer(Initializers& inits, Type* type,
                                     std::int32_t offset) {
  if (!type->ArrayGetElementType()->IsIntegerTy()) {
    return false;
  }

  auto has_brace{Try(Tag::kLeftBrace)};
  if (!Test(Tag::kStringLiteral)) {
    if (has_brace) {
      PutBack();
    }
    return false;
  }

  auto str_node{ParseStringLiteral()};

  if (has_brace) {
    Try(Tag::kComma);
    Expect(Tag::kRightBrace);
  }

  if (!type->IsComplete()) {
    type->ArraySetNumElements(str_node->GetType()->ArrayGetNumElements());
    type->SetComplete(true);
  }

  // TODO 若数组大小已知, 则它可以比字符串字面量的大小少一,
  // 此情况下空终止字符被忽略
  if (str_node->GetType()->ArrayGetNumElements() >
      type->ArrayGetNumElements()) {
    Error(str_node->GetLoc(),
          "initializer-string for char array is too long '{}' to '{}",
          str_node->GetType()->ArrayGetNumElements(),
          type->ArrayGetNumElements());
  }

  if (str_node->GetType()->ArrayGetElementType()->GetWidth() !=
      type->ArrayGetElementType()->GetWidth()) {
    Error(str_node->GetLoc(), "Different character types '{}' vs '{}",
          str_node->GetType()->ArrayGetElementType()->ToString(),
          type->ArrayGetElementType()->ToString());
  }

  auto width{type->ArrayGetElementType()->GetWidth()};
  auto size{str_node->GetType()->ArrayGetNumElements()};
  auto str{str_node->GetVal().c_str()};

  switch (width) {
    case 1:
      for (std::size_t i{}; i < size; ++i) {
        auto ptr{reinterpret_cast<const std::uint8_t*>(str)};
        auto char_type{ArithmeticType::Get(kChar | kUnsigned)};
        auto value{
            ConstantExpr::Get(char_type, static_cast<std::uint64_t>(*ptr))};
        inits.AddInit({char_type, offset, value});
        offset += 1;
        str += 1;
      }
      break;
    case 2:
      for (std::size_t i{}; i < size; ++i) {
        auto ptr{reinterpret_cast<const std::uint16_t*>(str)};
        auto char_type{ArithmeticType::Get(kShort | kUnsigned)};
        auto value{
            ConstantExpr::Get(char_type, static_cast<std::uint64_t>(*ptr))};
        inits.AddInit({char_type, offset, value});
        offset += 2;
        str += 2;
      }
      break;
    case 4:
      for (std::size_t i{}; i < size; ++i) {
        auto ptr{reinterpret_cast<const std::uint32_t*>(str)};
        auto char_type{ArithmeticType::Get(kInt | kUnsigned)};
        auto value{
            ConstantExpr::Get(char_type, static_cast<std::uint64_t>(*ptr))};
        inits.AddInit({char_type, offset, value});
        offset += 4;
        str += 4;
      }
      break;
    default:
      assert(false);
  }

  return true;
}

void Parser::ParseStructInitializer(Initializers& inits, Type* type,
                                    std::int32_t offset, bool designated) {
  auto has_brace{Try(Tag::kLeftBrace)};
  auto member_iter{std::begin(type->StructGetMembers())};

  while (true) {
    if (Test(Tag::kRightBrace)) {
      if (has_brace) {
        Next();
      }
      return;
    }

    // TODO ???
    if (!designated && !has_brace &&
        (Test(Tag::kPeriod) || Test(Tag::kLeftSquare))) {
      PutBack();
      return;
    }

    if ((designated = Try(Tag::kPeriod))) {
      auto tok{Expect(Tag::kIdentifier)};
      auto name{tok.GetIdentifier()};

      if (!type->StructGetMember(name)) {
        Error(tok, "member '{}' not found", name);
      }

      member_iter = ParseStructDesignator(type, name);
    }

    if (member_iter == std::end(type->StructGetMembers())) {
      break;
    }

    if ((*member_iter)->IsAnonymous()) {
      if (designated) {
        PutBack();
        PutBack();
      }

      ParseInitializer(inits, (*member_iter)->GetType(), offset, designated,
                       false);
    } else {
      ParseInitializer(inits, (*member_iter)->GetType(),
                       offset + (*member_iter)->GetOffset(), designated, false);
    }

    designated = false;
    ++member_iter;

    if (!type->IsStructTy()) {
      break;
    }

    if (!has_brace && member_iter == std::end(type->StructGetMembers())) {
      break;
    }

    if (!Try(Tag::kComma)) {
      if (has_brace) {
        Expect(Tag::kRightBrace);
      }
      return;
    }
  }

  if (has_brace) {
    Try(Tag::kComma);
    if (!Try(Tag::kRightBrace)) {
      Error(Peek(), "excess members in struct initializer");
    }
  }
}

auto Parser::ParseStructDesignator(Type* type, const std::string& name)
    -> decltype(std::begin(type->StructGetMembers())) {
  auto iter{std::begin(type->StructGetMembers())};

  for (; iter != std::end(type->StructGetMembers()); ++iter) {
    if ((*iter)->IsAnonymous()) {
      auto anonymous_type{(*iter)->GetType()};
      if (anonymous_type->StructGetMember(name)) {
        return iter;
      }
    } else if ((*iter)->GetName() == name) {
      return iter;
    }
  }

  assert(false);
  return iter;
}

/*
 * GNU 扩展
 */
// attribute-specifier:
//  __ATTRIBUTE__ '(' '(' attribute-list-opt ')' ')'
//
// attribute-list:
//  attribute-opt
//  attribute-list ',' attribute-opt
//
// attribute:
//  attribute-name
//  attribute-name '(' ')'
//  attribute-name '(' parameter-list ')'
//
// attribute-name:
//  identifier
//
// parameter-list:
//  identifier
//  identifier ',' expression-list
//  expression-list-opt
//
// expression-list:
//  expression
//  expression-list ',' expression
// 可以有多个
void Parser::TryParseAttributeSpec() {
  while (Try(Tag::kAttribute)) {
    Expect(Tag::kLeftParen);
    Expect(Tag::kLeftParen);

    ParseAttributeList();

    Expect(Tag::kRightParen);
    Expect(Tag::kRightParen);
  }
}

void Parser::ParseAttributeList() {
  while (!Test(Tag::kRightParen)) {
    ParseAttribute();

    if (!Test(Tag::kRightParen)) {
      Expect(Tag::kComma);
    }
  }
}

void Parser::ParseAttribute() {
  Expect(Tag::kIdentifier);

  if (Try(Tag::kLeftParen)) {
    ParseAttributeParamList();
    Expect(Tag::kRightParen);
  }
}

void Parser::ParseAttributeParamList() {
  if (Try(Tag::kIdentifier)) {
    if (Try(Tag::kComma)) {
      ParseAttributeExprList();
    }
  } else {
    ParseAttributeExprList();
  }
}

void Parser::ParseAttributeExprList() {
  while (!Test(Tag::kRightParen)) {
    ParseExpr();

    if (!Test(Tag::kRightParen)) {
      Expect(Tag::kComma);
    }
  }
}

void Parser::TryParseAsm() {
  if (Try(Tag::kAsm)) {
    Expect(Tag::kLeftParen);
    ParseStringLiteral();
    Expect(Tag::kRightParen);
  }
}

QualType Parser::ParseTypeof() {
  Expect(Tag::kLeftParen);

  QualType type;

  if (!IsTypeName(Peek())) {
    auto expr{ParseExpr()};
    type = expr->GetQualType();
  } else {
    type = ParseTypeName();
  }

  Expect(Tag::kRightParen);

  return type;
}

Expr* Parser::ParseStmtExpr() {
  auto block{ParseCompoundStmt()};
  Expect(Tag::kRightParen);
  return MakeAstNode<StmtExpr>(block);
}

/*
 * built in
 */
Expr* Parser::ParseOffsetof() {
  Expect(Tag::kLeftParen);

  if (!IsTypeName(Peek())) {
    Error(loc_, "expect type name");
  }
  auto type{ParseTypeName()};

  Expect(Tag::kComma);
  auto name{Expect(Tag::kIdentifier).GetIdentifier()};
  Expect(Tag::kRightParen);

  return MakeAstNode<ConstantExpr>(
      ArithmeticType::Get(kLong | kUnsigned),
      static_cast<std::uint64_t>(type->StructGetMember(name)->GetOffset()));
}

Expr* Parser::ParseRegClass() {
  Expect(Tag::kLeftParen);

  auto expr{ParseExpr()};
  Expect(Tag::kRightParen);

  assert(expr->Kind() == AstNodeType::kTypeCastExpr);
  assert(expr->GetType()->IsPointerTy());

  auto type{expr->GetType()->PointerGetElementType()};
  if (type->IsIntegerTy() || type->IsPointerTy()) {
    return MakeAstNode<ConstantExpr>(0);
  } else if (type->IsFloatPointTy()) {
    return MakeAstNode<ConstantExpr>(1);
  } else if (type->IsStructOrUnionTy()) {
    return MakeAstNode<ConstantExpr>(2);
  } else {
    assert(false);
    return nullptr;
  }
}

}  // namespace kcc
