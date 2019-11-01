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
  std::shared_ptr<TranslationUnit> ParseTranslationUnit();

 private:
  std::shared_ptr<Stmt> ParseExternalDecl();
  std::shared_ptr<Type> ParseStructUnionSpec(bool is_struct);
  std::shared_ptr<Type> ParseEnumSpec();
  std::int32_t ParseAlignas();

  std::shared_ptr<CompoundStmt> ParseDecl();
  void ParseStaticAssertDecl();
  std::shared_ptr<Type> ParseDeclSpec(std::uint32_t &storage_class_spec,
                                      std::uint32_t &func_spec,
                                      std::int32_t &align);
  void ParseDeclarator(std::string &name, std::shared_ptr<Type> &base_type);
  void ParseDirectDeclarator(std::string &name,
                             std::shared_ptr<Type> &base_type);
  void ParseDirectDeclaratorTail(std::shared_ptr<Type> &base_type);
  std::shared_ptr<CompoundStmt> ParseInitDeclaratorList(
      std::shared_ptr<Type> &base_type, std::uint32_t storage_class_spec,
      std::uint32_t func_spec, std::int32_t align);
  std::shared_ptr<Decl> ParseInitDeclarator(std::shared_ptr<Type> &base_type,
                                            std::uint32_t storage_class_spec,
                                            std::uint32_t func_spec,
                                            std::int32_t align);
  std::shared_ptr<Decl> MakeDeclarator(const std::string &name,
                                       const std::shared_ptr<Type> &base_type,
                                       std::uint32_t storage_class_spec,
                                       std::uint32_t func_spec,
                                       std::int32_t align);
  std::set<Initializer> ParseInitializer();

  bool HasNext();
  Token Peek();
  Token PeekPrev();
  Token Next();
  void PutBack();
  bool Test(Tag tag);
  bool Try(Tag tag);
  void Expect(Tag tag);
  void MarkLoc(const SourceLocation &loc);

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

  std::shared_ptr<StringLiteral> ParseStringLiteral();
  std::string ConcatStr();

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
