//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_PARSE_H_
#define KCC_SRC_PARSE_H_

#include <memory>
#include <vector>

#include "ast.h"
#include "token.h"

namespace kcc {

class Parser {
 public:
  explicit Parser(std::vector<Token> tokens);
  std::shared_ptr<TranslationUnit> Parse();

 private:
  bool HasNext();
  Token Peek();
  Token PeekPrev();
  Token Next();
  void PutBack();
  bool Test(Tag tag);
  bool Try(Tag tag);
  void Expect(Tag tag);
  void MarkLoc(const SourceLocation &loc);

  void ParseStaticAssert();

  std::shared_ptr<Expr> ParseCastExpr();

  std::shared_ptr<Expr> ParseMultiplicativeExpr();
  std::shared_ptr<Expr> ParseAdditiveExpr();
  std::shared_ptr<Expr> ParseShiftExpr();
  std::shared_ptr<Expr> ParseRelationExpr();
  std::shared_ptr<Expr> ParseEqualityExpr();
  std::shared_ptr<Expr> ParseBitwiseAndExpr();
  std::shared_ptr<Expr> ParseBitwiseXorExpr();
  std::shared_ptr<Expr> ParseBitwiseOrExpr();
  std::shared_ptr<Expr> ParseLogicalAndExpr();
  std::shared_ptr<Expr> ParseLogicalOrExpr();
  std::shared_ptr<Expr> ParseConditionExpr();
  std::shared_ptr<Expr> ParseAssignExpr();
  std::shared_ptr<Expr> ParseCommaExpr();
  std::shared_ptr<Expr> ParseExpr();

  std::vector<Token> tokens_;
  decltype(tokens_)::size_type index_{};

  std::shared_ptr<TranslationUnit> unit_{std::make_shared<TranslationUnit>()};

  SourceLocation loc_;

  template <typename T, typename... Args>
  std::shared_ptr<T> MakeAstNode(Args &&... args) {
    auto t{std::make_shared<T>(std::forward<Args>(args)...)};
    t->SetLoc(loc_);
    return t;
  }
};

}  // namespace kcc

#endif  // KCC_SRC_PARSE_H_
