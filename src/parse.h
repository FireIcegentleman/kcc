//
// Created by kaiser on 2019/10/31.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/IR/Constants.h>

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
  bool HasNext();
  const Token& Peek();
  const Token& Next();
  void PutBack();
  bool Test(Tag tag);
  bool Try(Tag tag);
  const Token& Expect(Tag tag);

  void EnterBlock(Type* func_type = nullptr);
  void ExitBlock();
  void EnterFunc(IdentifierExpr* ident);
  void ExitFunc();
  void EnterProto();
  void ExitProto();
  bool IsTypeName(const Token& tok);
  bool IsDecl(const Token& tok);
  std::int64_t ParseInt64Constant();
  LabelStmt* FindLabel(const std::string& name) const;
  static auto GetStructDesignator(Type* type, const std::string& name)
      -> decltype(std::begin(type->StructGetMembers()));
  Declaration* MakeDeclaration(const Token& token, QualType type,
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
  StringLiteralExpr* ParseStringLiteral(bool handle_escape = true);
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
  void ParseBitField(StructType* type, const Token& tok, QualType member_type);
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
  void ParseInitDeclaratorSub(Declaration* decl);
  void ParseDeclarator(Token& tok, QualType& base_type);
  void ParsePointer(QualType& type);
  std::uint32_t ParseTypeQualList();
  void ParseDirectDeclarator(Token& tok, QualType& base_type);
  void ParseDirectDeclaratorTail(QualType& base_type);
  std::int64_t ParseArrayLength();
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
  llvm::Constant* ParseInitializer(std::vector<Initializer>& inits,
                                   QualType type, bool designated,
                                   bool force_brace);
  void ParseArrayInitializer(std::vector<Initializer>& inits, Type* type,
                             std::int32_t designated);
  llvm::Constant* ParseLiteralInitializer(Type* type);
  void ParseStructInitializer(std::vector<Initializer>& inits, Type* type,
                              bool designated);

  /*
   * ConstantInit
   */
  llvm::Constant* ParseConstantInitializer(QualType type, bool designated,
                                           bool force_brace);
  llvm::Constant* ParseConstantArrayInitializer(Type* type,
                                                std::int32_t designated);
  llvm::Constant* ParseConstantLiteralInitializer(Type* type);
  llvm::Constant* ParseConstantStructInitializer(Type* type, bool designated);

  /*
   * GNU 扩展
   */
  void TryParseAttributeSpec();
  void ParseAttributeList();
  void ParseAttribute();
  void ParseAttributeParamList();
  void ParseAttributeExprList();
  void TryParseAsm();
  QualType ParseTypeof();
  Expr* TryParseStmtExpr();
  Expr* ParseStmtExpr();
  Expr* ParseTypeid();

  /*
   * built in
   */
  Expr* ParseOffsetof();
  Expr* ParseHugeVal();
  Expr* ParseInff();
  void AddBuiltin();

  TranslationUnit* unit_;

  std::vector<Token> tokens_;
  decltype(tokens_)::size_type index_{};

  FuncDef* func_def_{};
  Scope* scope_{Scope::Get(nullptr, kFile)};

  std::unordered_map<std::string, LabelStmt*> labels_;
  std::vector<GotoStmt*> gotos_;

  // 用于将块作用与的复合字面量加入块中
  std::stack<CompoundStmt*> compound_stmt_;

  // 非常量初始化时记录索引
  std::vector<std::tuple<Type*, std::int32_t, std::int8_t, std::int8_t>>
      indexs_;
};

}  // namespace kcc
