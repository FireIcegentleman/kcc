//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_PARSE_H_
#define KCC_SRC_PARSE_H_

#include <map>
#include <memory>
#include <vector>

#include "ast.h"
#include "scope.h"
#include "token.h"

namespace kcc {

class Parser {
 public:
  explicit Parser(std::vector<Token> tokens);
  std::shared_ptr<TranslationUnit> ParseTranslationUnit();

 private:
  std::shared_ptr<ExtDecl> ParseExternalDecl();
  std::shared_ptr<FuncDef> ParseFuncDef(
      const std::shared_ptr<Declaration> &decl);

  std::shared_ptr<CompoundStmt> ParseDecl(bool maybe_func_def = false);
  QualType ParseDeclSpec(std::uint32_t *storage_class_spec,
                         std::uint32_t *func_spec, std::int32_t *align);

  std::shared_ptr<Type> ParseStructUnionSpec(bool is_struct);
  std::shared_ptr<Type> ParseEnumSpec();
  void ParseEnumerator(std::shared_ptr<Type> type);
  std::int32_t ParseAlignas();
  std::size_t ParseArrayLength();
  void EnterProto();
  void ExitProto();
  std::pair<std::vector<std::shared_ptr<Object>>, bool> ParseParamTypeList();
  bool IsTypeName(const Token &tok);
  bool IsDecl(const Token &tok);
  std::shared_ptr<Type> ParseTypeName();
  void ParseStructDeclList(std::shared_ptr<StructType> base_type);
  std::shared_ptr<Object> ParseParamDecl();
  void ParseAbstractDeclarator(std::shared_ptr<Type> &type);
  void ParsePointer(std::shared_ptr<Type> &type);
  void ParseTypeQualList(QualType &type);
  void ParseDirectAbstractDeclarator(QualType &type);

  void ParseStaticAssertDecl();

  void ParseDeclarator(Token &tok, QualType &base_type);
  void ParseDirectDeclarator(Token &tok, QualType &base_type);
  void ParseDirectDeclaratorTail(QualType &base_type);
  std::shared_ptr<CompoundStmt> ParseInitDeclaratorList(QualType &base_type);
  std::shared_ptr<Declaration> ParseInitDeclarator(QualType &base_type);
  std::shared_ptr<Declaration> MakeDeclarator(const Token &tok,
                                              const QualType &type);
  std::shared_ptr<Expr> ParseConstantExpr();
  std::shared_ptr<Constant> ParseStringLiteral(bool handle_escape);
  std::set<Initializer> ParseInitDeclaratorSub(
      std::shared_ptr<Identifier> ident);
  void ParseInitializer(std::set<Initializer> &inits, QualType type,
                        std::int32_t offset, bool designated, bool force_brace);

  bool HasNext();
  Token Peek();
  Token PeekPrev();
  Token Next();
  void PutBack();
  bool Test(Tag tag);
  bool Try(Tag tag);
  Token Expect(Tag tag);

  std::shared_ptr<Expr> ParseConstant();
  std::shared_ptr<Expr> ParseFloat();
  std::shared_ptr<Expr> ParseInteger();
  std::shared_ptr<Expr> ParseCharacter();
  std::shared_ptr<Expr> ParseSizeof();
  std::shared_ptr<Expr> ParseAlignof();
  std::shared_ptr<Expr> ParsePrimaryExpr();
  std::shared_ptr<Expr> ParsePostfixExprTail(std::shared_ptr<Expr> expr);
  std::shared_ptr<Expr> ParsePostfixExpr();
  std::shared_ptr<Expr> ParseUnaryExpr();
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

  void EnterBlock(QualType func_type = {});
  void ExitBlock();
  void EnterFunc(const std::shared_ptr<Identifier> &ident);
  void ExitFunc();
  std::shared_ptr<Stmt> ParseStmt();
  std::shared_ptr<LabelStmt> ParseLabelStmt();
  std::shared_ptr<CaseStmt> ParseCaseStmt();
  std::shared_ptr<DefaultStmt> ParseDefaultStmt();
  std::shared_ptr<CompoundStmt> ParseCompoundStmt(
      std::shared_ptr<Type> func_type = nullptr);
  std::shared_ptr<ExprStmt> ParseExprStmt();
  std::shared_ptr<IfStmt> ParseIfStmt();
  std::shared_ptr<SwitchStmt> ParseSwitchStmt();
  std::shared_ptr<WhileStmt> ParseWhileStmt();
  std::shared_ptr<DoWhileStmt> ParseDoWhileStmt();
  std::shared_ptr<ForStmt> ParseForStmt();
  std::shared_ptr<GotoStmt> ParseGotoStmt();
  std::shared_ptr<ContinueStmt> ParseContinueStmt();
  std::shared_ptr<BreakStmt> ParseBreakStmt();
  std::shared_ptr<ReturnStmt> ParseReturnStmt();

  void TryAttributeSpec();
  void ParseAttributeList();
  void ParseAttribute();
  void ParseAttributeParamList();
  void ParseAttributeExprList();
  void TryAsm();

  std::vector<Token> tokens_;
  decltype(tokens_)::size_type index_{};

  std::shared_ptr<Scope> curr_scope_{std::make_shared<Scope>(nullptr, kFile)};
  std::map<std::string, std::shared_ptr<LabelStmt>> curr_labels_;
  std::shared_ptr<FuncDef> curr_func_def_;
};

}  // namespace kcc

#endif  // KCC_SRC_PARSE_H_
