//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_PARSE_H_
#define KCC_SRC_PARSE_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ast.h"
#include "location.h"
#include "scope.h"
#include "token.h"
#include "type.h"

namespace kcc {

class Parser {
 public:
  explicit Parser(std::vector<Token> tokens);
  TranslationUnit* ParseTranslationUnit();

 private:
  template <typename T, typename... Args>
  T* MakeAstNode(Args&&... args);

  bool HasNext();
  Token Peek();
  Token Next();
  void PutBack();
  bool Test(Tag tag);
  bool Try(Tag tag);
  Token Expect(Tag tag);
  void MarkLoc();

  void EnterBlock(Type* func_type = nullptr);
  void ExitBlock();
  void EnterFunc(IdentifierExpr* ident);
  void ExitFunc();
  void EnterProto();
  void ExitProto();
  bool IsTypeName(const Token& tok);
  bool IsDecl(const Token& tok);
  Declaration* MakeDeclaration(const std::string& name, QualType type,
                               std::uint32_t storage_class_spec,
                               std::uint32_t func_spec, std::int32_t align);

  /*
   * ExtDecl
   */
  ExtDecl* ParseExternalDecl();
  FuncDef* ParseFuncDef(const Declaration* decl);

  /*
   * Expr
   */
  Expr* ParseExpr();
  Expr* ParseAssignExpr();
  Expr* ParseConditionExpr();
  Expr* ParseLogicalOrExpr();
  Expr* ParseLogicalAndExpr();
  Expr* ParseInclusiveOrExpr();
  Expr* ParseExclusiveOrExpr();
  Expr* ParseAndExpr();
  Expr* ParseEqualityExpr();
  Expr* ParseRelationExpr();
  Expr* ParseShiftExpr();
  Expr* ParseAdditiveExpr();
  Expr* ParseMultiplicativeExpr();
  Expr* ParseCastExpr();
  Expr* ParseUnaryExpr();
  Expr* ParseSizeof();
  Expr* ParseAlignof();
  Expr* ParsePostfixExpr();
  Expr* TryParseCompoundLiteral();
  Expr* ParseCompoundLiteral(QualType type);
  Expr* ParsePostfixExprTail(Expr* expr);
  Expr* ParseIndexExpr(Expr* expr);
  Expr* ParseFuncCallExpr(Expr* expr);
  Expr* ParseMemberRefExpr(Expr* expr);
  Expr* ParsePrimaryExpr();
  Expr* ParseConstant();
  Expr* ParseCharacter();
  Expr* ParseInteger();
  Expr* ParseFloat();
  StringLiteralExpr* ParseStringLiteral();
  Expr* ParseGenericSelection();
  Expr* ParseConstantExpr();

  /*
   * Stmt
   */
  Stmt* ParseStmt();
  Stmt* ParseLabelStmt();
  Stmt* ParseCaseStmt();
  Stmt* ParseDefaultStmt();
  CompoundStmt* ParseCompoundStmt(Type* func_type = nullptr);
  Stmt* ParseExprStmt();
  Stmt* ParseIfStmt();
  Stmt* ParseSwitchStmt();
  Stmt* ParseWhileStmt();
  Stmt* ParseDoWhileStmt();
  Stmt* ParseForStmt();
  Stmt* ParseGotoStmt();
  Stmt* ParseContinueStmt();
  Stmt* ParseBreakStmt();
  Stmt* ParseReturnStmt();

  /*
   * Decl
   */
  CompoundStmt* ParseDecl(bool maybe_func_def = false);
  void ParseStaticAssertDecl();

  /*
   * Decl Spec
   */
  QualType ParseDeclSpec(std::uint32_t* storage_class_spec,
                         std::uint32_t* func_spec, std::int32_t* align);
  Type* ParseStructUnionSpec(bool is_struct);
  void ParseStructDeclList(StructType* type);
  Type* ParseEnumSpec();
  void ParseEnumerator();
  std::int32_t ParseAlignas();

  /*
   * Declarator
   */
  CompoundStmt* ParseInitDeclaratorList(QualType& base_type,
                                        std::uint32_t storage_class_spec,
                                        std::uint32_t func_spec,
                                        std::int32_t align);
  Declaration* ParseInitDeclarator(QualType& base_type,
                                   std::uint32_t storage_class_spec,
                                   std::uint32_t func_spec, std::int32_t align);
  void ParseDeclarator(Token& tok, QualType& base_type);
  void ParsePointer(QualType& type);
  std::uint32_t ParseTypeQualList();
  void ParseDirectDeclarator(Token& tok, QualType& base_type);
  void ParseDirectDeclaratorTail(QualType& base_type);
  std::size_t ParseArrayLength();
  std::pair<std::vector<ObjectExpr*>, bool> ParseParamTypeList();
  ObjectExpr* ParseParamDecl();

  /*
   * type name
   */
  QualType ParseTypeName();
  void ParseAbstractDeclarator(QualType& type);
  void ParseDirectAbstractDeclarator(QualType& type);

  /*
   * Init
   */
  std::set<Initializer> ParseInitDeclaratorSub(IdentifierExpr* ident);
  void ParseInitializer(std::set<Initializer>& inits, QualType type,
                        std::int32_t offset, bool designated, bool force_brace);
  void ParseArrayInitializer(std::set<Initializer>& inits, Type* type,
                             std::int32_t offset, std::int32_t designated);
  bool ParseLiteralInitializer(std::set<Initializer>& inits, Type* type,
                               std::int32_t offset);
  void ParseStructInitializer(std::set<Initializer>& inits, Type* type,
                              std::int32_t offset, bool designated);
  static auto ParseStructDesignator(Type* type, const std::string& name)
      -> decltype(std::begin(type->StructGetMembers()));

  /*
   * GNU 扩展
   */
  void TryParseAttributeSpec();
  void ParseAttributeList();
  void ParseAttribute();
  void ParseAttributeParamList();
  void ParseAttributeExprList();
  void TryParseAsm();

  std::vector<Token> tokens_;
  decltype(tokens_)::size_type index_{};

  Location loc_;

  FuncDef* curr_func_def_{};
  Scope* curr_scope_{Scope::Get(nullptr, kFile)};
  std::map<std::string, LabelStmt*> curr_labels_;

  TranslationUnit* unit_{MakeAstNode<TranslationUnit>()};
};

template <typename T, typename... Args>
T* Parser::MakeAstNode(Args&&... args) {
  auto t{T::Get(std::forward<Args>(args)...)};
  t->SetLoc(loc_);
  t->Check();
  return t;
}

}  // namespace kcc

#endif  // KCC_SRC_PARSE_H_
