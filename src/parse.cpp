//
// Created by kaiser on 2019/10/31.
//

#include "parse.h"

#include "error.h"

namespace kcc {

Parser::Parser(std::vector<Token> tokens) : tokens_{std::move(tokens)} {}

std::shared_ptr<TranslationUnit> Parser::Parse() {
  while (HasNext()) {
    if (Try(Tag::kStaticAssert)) {
      ParseStaticAssert();
      continue;
    } else if (Try(Tag::kSemicolon)) {
      Warning(PeekPrev().GetLoc(), "extra ';' outside of a function");
      continue;
    }

    // temp
    ParseExpr();
  }

  return std::move(unit_);
}

bool Parser::HasNext() { return !Peek().TagIs(Tag::kEof); }

Token Parser::Peek() { return tokens_[index_]; }

Token Parser::Next() { return tokens_[index_++]; }

void Parser::PutBack() { --index_; }

bool Parser::Test(Tag tag) { return Peek().TagIs(tag); }

bool Parser::Try(Tag tag) {
  if (Test(tag)) {
    Next();
    return true;
  } else {
    return false;
  }
}

void Parser::ParseStaticAssert() { Expect(Tag::kLeftParen); }

Token Parser::PeekPrev() { return tokens_[index_ - 1]; }

void Parser::Expect(Tag tag) {
  if (!Try(tag)) {
    Error(tag, Peek());
  }
}

std::shared_ptr<Expr> Parser::ParseExpr() { return ParseCommaExpr(); }

std::shared_ptr<Expr> Parser::ParseCommaExpr() {
  auto lhs{ParseAssignExpr()};

  MarkLoc(Peek().GetLoc());
  while (Try(Tag::kComma)) {
    auto rhs{ParseAssignExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kComma, std::move(lhs), rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseAssignExpr() {
  auto lhs{ParseConditionExpr()};
  std::shared_ptr<Expr> rhs;

  auto tok{Next()};
  MarkLoc(tok.GetLoc());

  switch (tok.GetTag()) {
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

  return MakeAstNode<BinaryOpExpr>(Tag::kEqual, lhs, rhs);
}

std::shared_ptr<Expr> Parser::ParseConditionExpr() {
  auto cond{ParseLogicalOrExpr()};
  MarkLoc(Peek().GetLoc());

  // 条件运算符 ? 与 : 之间的表达式分析为如同加括号，忽略其相对于 ?: 的优先级
  if (Try(Tag::kQuestion)) {
    // GNU 扩展
    // a ?: b 相当于 a ? a: c
    auto true_expr{Test(Tag::kColon) ? cond : ParseExpr()};
    Expect(Tag::kColon);
    auto false_expr{ParseExpr()};

    return MakeAstNode<ConditionOpExpr>(cond, true_expr, false_expr);
  }

  return cond;
}

std::shared_ptr<Expr> Parser::ParseLogicalOrExpr() {
  auto lhs{ParseLogicalAndExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kPipePipe)) {
    auto rhs{ParseLogicalAndExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kPipePipe, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseLogicalAndExpr() {
  auto lhs{ParseBitwiseOrExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kAmpAmp)) {
    auto rhs{ParseBitwiseOrExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kAmpAmp, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseBitwiseOrExpr() {
  auto lhs{ParseBitwiseXorExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kPipe)) {
    auto rhs{ParseBitwiseXorExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kPipe, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseBitwiseXorExpr() {
  auto lhs{ParseBitwiseAndExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kCaret)) {
    auto rhs{ParseBitwiseAndExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kCaret, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseBitwiseAndExpr() {
  auto lhs{ParseEqualityExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kAmp)) {
    auto rhs{ParseEqualityExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kAmp, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseEqualityExpr() {
  auto lhs{ParseRelationExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kEqual)) {
      auto rhs{ParseRelationExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kEqual, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kExclaimEqual)) {
      auto rhs{ParseRelationExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kExclaimEqual, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseRelationExpr() {
  auto lhs{ParseShiftExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kLess)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLess, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kLessEqual)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLessEqual, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kGreater)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreater, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kGreaterEqual)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreaterEqual, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseShiftExpr() {
  auto lhs{ParseAdditiveExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kLessLess)) {
      auto rhs{ParseAdditiveExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLessLess, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kGreaterGreater)) {
      auto rhs{ParseAdditiveExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreaterGreater, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseAdditiveExpr() {
  auto lhs{ParseMultiplicativeExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kPlus)) {
      auto rhs{ParseMultiplicativeExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kPlus, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kMinus)) {
      auto rhs{ParseMultiplicativeExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kMinus, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseMultiplicativeExpr() {
  auto lhs{ParseCastExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kStar)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kStar, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kSlash)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kSlash, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kPercent)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kPercent, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseCastExpr() {
  auto tok{Next()};
  MarkLoc(tok.GetLoc());

  if()
}

}  // namespace kcc
