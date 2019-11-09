//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_PARSE_H_
#define KCC_SRC_PARSE_H_

#include <map>
#include <vector>

#include "ast.h"
#include "scope.h"
#include "token.h"

namespace kcc {

class Parser {
 public:
  explicit Parser(std::vector<Token> tokens);
  TranslationUnit* ParseTranslationUnit();

 private:
  ExtDecl* ParseExternalDecl();
  FuncDef* ParseFuncDef(const Declaration* decl);

  CompoundStmt* ParseDecl(bool maybe_func_def = false);
  QualType ParseDeclSpec(std::uint32_t* storage_class_spec,
                         std::uint32_t* func_spec, std::int32_t* align);
  CompoundStmt* ParseInitDeclaratorList(QualType& base_type,
                                        std::uint32_t storage_class_spec,
                                        std::uint32_t func_spec,
                                        std::int32_t align);
  Declaration* ParseInitDeclarator(QualType& base_type,
                                   std::uint32_t storage_class_spec,
                                   std::uint32_t func_spec, std::int32_t align);
  void ParseDeclarator(Token& tok, QualType& base_type);
  void ParseDirectDeclarator(Token& tok, QualType& base_type);
  void ParseDirectDeclaratorTail(QualType& base_type);
  void ParsePointer(QualType& type);

  Type* ParseStructUnionSpec(bool is_struct);
  Type* ParseEnumSpec();
  void ParseEnumerator(Type* type);
  std::int32_t ParseAlignas();
  std::size_t ParseArrayLength();
  void EnterProto();
  void ExitProto();
  std::pair<std::vector<Object*>, bool> ParseParamTypeList();
  bool IsTypeName(const Token& tok);
  bool IsDecl(const Token& tok);
  QualType ParseTypeName();
  void ParseStructDeclList(StructType* base_type);
  Object* ParseParamDecl();
  void ParseAbstractDeclarator(QualType& type);

  std::uint32_t ParseTypeQualList();
  void ParseDirectAbstractDeclarator(QualType& type);

  void ParseStaticAssertDecl();

  Declaration* MakeDeclarator(const Token& tok, const QualType& type,
                              std::uint32_t storage_class_spec,
                              std::uint32_t func_spec, std::int32_t align);
  Expr* ParseConstantExpr();
  Constant* ParseStringLiteral(bool handle_escape);
  std::set<Initializer> ParseInitDeclaratorSub(Identifier* ident);
  void ParseInitializer(std::set<Initializer>& inits, QualType type,
                        std::int32_t offset, bool designated, bool force_brace);

  bool HasNext();
  Token Peek();
  Token PeekPrev();
  Token Next();
  void PutBack();
  bool Test(Tag tag);
  bool Try(Tag tag);
  Token Expect(Tag tag);

  Expr* ParseConstant();
  Expr* ParseFloat();
  Expr* ParseInteger();
  Expr* ParseCharacter();
  Expr* ParseSizeof();
  Expr* ParseAlignof();
  Expr* ParsePrimaryExpr();
  Expr* ParsePostfixExprTail(Expr* expr);
  Expr* ParsePostfixExpr();
  Expr* ParseUnaryExpr();
  Expr* ParseCastExpr();
  Expr* ParseMultiplicativeExpr();
  Expr* ParseAdditiveExpr();
  Expr* ParseShiftExpr();
  Expr* ParseRelationExpr();
  Expr* ParseEqualityExpr();
  Expr* ParseBitwiseAndExpr();
  Expr* ParseBitwiseXorExpr();
  Expr* ParseBitwiseOrExpr();
  Expr* ParseLogicalAndExpr();
  Expr* ParseLogicalOrExpr();
  Expr* ParseConditionExpr();
  Expr* ParseAssignExpr();
  Expr* ParseCommaExpr();
  Expr* ParseExpr();

  void EnterBlock(Type* func_type = nullptr);
  void ExitBlock();
  void EnterFunc(Identifier* ident);
  void ExitFunc();
  Stmt* ParseStmt();
  LabelStmt* ParseLabelStmt();
  CaseStmt* ParseCaseStmt();
  DefaultStmt* ParseDefaultStmt();
  CompoundStmt* ParseCompoundStmt(Type* func_type = nullptr);
  ExprStmt* ParseExprStmt();
  IfStmt* ParseIfStmt();
  SwitchStmt* ParseSwitchStmt();
  WhileStmt* ParseWhileStmt();
  DoWhileStmt* ParseDoWhileStmt();
  ForStmt* ParseForStmt();
  GotoStmt* ParseGotoStmt();
  ContinueStmt* ParseContinueStmt();
  BreakStmt* ParseBreakStmt();
  ReturnStmt* ParseReturnStmt();

  void TryAttributeSpec();
  void ParseAttributeList();
  void ParseAttribute();
  void ParseAttributeParamList();
  void ParseAttributeExprList();
  void TryAsm();

  std::vector<Token> tokens_;
  decltype(tokens_)::size_type index_{};

  Scope* curr_scope_{Scope::Get(nullptr, kFile)};
  std::map<std::string, LabelStmt*> curr_labels_;
  FuncDef* curr_func_def_{};
};

}  // namespace kcc

#endif  // KCC_SRC_PARSE_H_
