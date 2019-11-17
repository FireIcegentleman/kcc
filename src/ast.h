//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_AST_H_
#define KCC_SRC_AST_H_

#include <cstdint>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#include <QMetaEnum>
#include <QObject>
#include <QString>

#include "location.h"
#include "token.h"
#include "type.h"

namespace kcc {

class CompoundStmt;
class Declaration;
class Visitor;

class AstNodeTypes : public QObject {
  Q_OBJECT
 public:
  enum Type {
    kUnaryOpExpr,
    kTypeCastExpr,
    kBinaryOpExpr,
    kConditionOpExpr,
    kFuncCallExpr,
    kConstantExpr,
    kStringLiteralExpr,
    kIdentifierExpr,
    kEnumeratorExpr,
    kObjectExpr,
    kStmtExpr,

    kLabelStmt,
    kCaseStmt,
    kDefaultStmt,
    kCompoundStmt,
    kExprStmt,
    kIfStmt,
    kSwitchStmt,
    kWhileStmt,
    kDoWhileStmt,
    kForStmt,
    kGotoStmt,
    kContinueStmt,
    kBreakStmt,
    kReturnStmt,

    kTranslationUnit,
    kDeclaration,
    kFuncDef,
  };

  Q_ENUM(Type)

  static QString ToString(Type type);
};

using AstNodeType = AstNodeTypes::Type;

class AstNode {
 public:
  virtual ~AstNode() = default;

  virtual AstNodeType Kind() const = 0;
  virtual void Accept(Visitor& visitor) const = 0;
  virtual void Check() = 0;

  QString KindStr() const;
  Location GetLoc() const;
  void SetLoc(const Location& loc);

 protected:
  AstNode() = default;

  Location loc_;
};

class Expr : public AstNode {
 public:
  virtual bool IsLValue() const = 0;

  QualType GetQualType() const;
  Type* GetType();
  const Type* GetType() const;

  bool IsConst() const;
  bool IsRestrict() const;

  void EnsureCompatible(QualType lhs, QualType rhs) const;
  void EnsureCompatibleOrVoidPtr(QualType lhs, QualType rhs) const;
  // 数组函数隐式转换
  static Expr* MayCast(Expr* expr);
  static Expr* MayCastTo(Expr* expr, QualType to);
  static Type* Convert(Expr*& lhs, Expr*& rhs);

 protected:
  explicit Expr(QualType type = {});

  QualType type_;
};

/*
 * ++ --
 * + - ~
 * !
 * * &
 */
class UnaryOpExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class ConstantInitExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static UnaryOpExpr* Get(Tag tag, Expr* expr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

 private:
  UnaryOpExpr(Tag tag, Expr* expr);

  void IncDecOpCheck();
  void UnaryAddSubOpCheck();
  void NotOpCheck();
  void LogicNotOpCheck();
  void DerefOpCheck();
  void AddrOpCheck();

  Tag op_;
  Expr* expr_;
};

class TypeCastExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class ConstantInitExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static TypeCastExpr* Get(Expr* expr, QualType to);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

 private:
  TypeCastExpr(Expr* expr, QualType to);

  Expr* expr_;
  QualType to_;
};

/*
 * =
 * + - * / % & | ^ << >>
 * && ||
 * == != < > <= >=
 * .
 * ,
 */
// 复合赋值运算符, [] , -> 均做了转换
class BinaryOpExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class ConstantInitExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static BinaryOpExpr* Get(Tag tag, Expr* lhs, Expr* rhs);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

 private:
  BinaryOpExpr(Tag tag, Expr* lhs, Expr* rhs);

  void AssignOpCheck();
  void AddOpCheck();
  void SubOpCheck();
  void MultiOpCheck();
  void BitwiseOpCheck();
  void ShiftOpCheck();
  void LogicalOpCheck();
  void EqualityOpCheck();
  void RelationalOpCheck();
  void MemberRefOpCheck();
  void CommaOpCheck();

  Tag op_;
  Expr* lhs_;
  Expr* rhs_;
};

class ConditionOpExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class ConstantInitExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static ConditionOpExpr* Get(Expr* cond, Expr* lhs, Expr* rhs);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

 private:
  ConditionOpExpr(Expr* cond, Expr* lhs, Expr* rhs);

  Expr* cond_;
  Expr* lhs_;
  Expr* rhs_;
};

class FuncCallExpr : public Expr {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static FuncCallExpr* Get(Expr* callee, std::vector<Expr*> args = {});

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  Type* GetFuncType() const;

 private:
  explicit FuncCallExpr(Expr* callee, std::vector<Expr*> args = {});

  Expr* callee_;
  std::vector<Expr*> args_;
};

class ConstantExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class ConstantInitExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static ConstantExpr* Get(std::int32_t val);
  static ConstantExpr* Get(Type* type, std::uint64_t val);
  static ConstantExpr* Get(Type* type, long double val);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  long GetIntegerVal() const;
  double GetFloatPointVal() const;

 private:
  ConstantExpr(std::int32_t val);
  ConstantExpr(Type* type, std::uint64_t val);
  ConstantExpr(Type* type, long double val);

  std::uint64_t integer_val_{};
  long double float_point_val_{};
};

class StringLiteralExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class ConstantInitExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static StringLiteralExpr* Get(const std::string& val);
  static StringLiteralExpr* Get(Type* type, const std::string& val);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  std::string GetVal() const;

 private:
  StringLiteralExpr(Type* type, const std::string& val);

  std::string val_;
};

enum Linkage { kNone, kInternal, kExternal };

// 标识符能指代下列类型的实体：
// 对象
// 函数
// struct / union / enum tag
// 结构体或联合体成员
// 枚举常量
// typedef 名
// 标号名
// 宏名
// 宏形参名
// 宏名或宏形参名以外的每个标识符都拥有作用域，并且可以拥有链接
class IdentifierExpr : public Expr {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static IdentifierExpr* Get(const std::string& name, QualType type,
                             enum Linkage linkage, bool is_type_name);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  enum Linkage GetLinkage() const;
  std::string GetName() const;
  bool IsTypeName() const;
  bool IsObject() const;
  bool IsEnumerator() const;

 protected:
  IdentifierExpr(const std::string& name, QualType type, enum Linkage linkage,
                 bool is_type_name);

  std::string name_;
  enum Linkage linkage_;
  bool is_type_name_;
};

class EnumeratorExpr : public IdentifierExpr {
  template <typename T>
  friend class CalcExpr;
  friend class ConstantInitExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static EnumeratorExpr* Get(const std::string& name, std::int32_t val);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  std::int32_t GetVal() const;

 private:
  EnumeratorExpr(const std::string& name, std::int32_t val);

  std::int32_t val_;
};

// C 中，一个对象是执行环境中数据存储的一个区域，其内容可以表示值
// 每个对象拥有
// 大小（可由 sizeof 确定）
// 对齐要求（可由 _Alignof 确定） (C11 起)
// 存储期（自动、静态、分配、线程局域）
// 生存期（等于存储期或临时）
// 有效类型（见下）
// 值（可以是不确定的）
// 可选项，表示该对象的标识符
class ObjectExpr : public IdentifierExpr {
  friend class JsonGen;
  friend class ConstantInitExpr;
  friend class CodeGen;

 public:
  static ObjectExpr* Get(const std::string& name, QualType type,
                         std::uint32_t storage_class_spec = 0,
                         enum Linkage linkage = kNone, bool anonymous = false);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

  bool IsStatic() const;
  bool IsExtern() const;
  void SetStorageClassSpec(std::uint32_t storage_class_spec);
  std::uint32_t GetStorageClassSpec();
  std::int32_t GetAlign() const;
  void SetAlign(std::int32_t align);
  std::int32_t GetOffset() const;
  void SetOffset(std::int32_t offset);
  Declaration* GetDecl();
  void SetDecl(Declaration* decl);
  bool HasInit() const;
  bool IsAnonymous() const;
  bool InGlobal() const;
  void SetLocalPtr(llvm::AllocaInst* local_ptr);
  void SetGlobalPtr(llvm::GlobalVariable* global_ptr);
  llvm::AllocaInst* GetLocalPtr();
  llvm::GlobalVariable* GetGlobalPtr();
  bool HasGlobalPtr() const;
  std::list<std::pair<Type*, std::int32_t>>& GetIndexs();
  void SetIndexs(const std::list<std::pair<Type*, std::int32_t>>& indexs);

 private:
  ObjectExpr(const std::string& name, QualType type,
             std::uint32_t storage_class_spec = 0, enum Linkage linkage = kNone,
             bool anonymous = false);

  bool anonymous_{};
  std::uint32_t storage_class_spec_{};
  std::int32_t align_{};
  std::int32_t offset_{};
  Declaration* decl_{};

  std::list<std::pair<Type*, std::int32_t>> indexs_;
  llvm::AllocaInst* local_ptr_{};
  llvm::GlobalVariable* global_ptr_{};
};

// GNU 扩展, 语句表达式, 它可以是常量表达式, 不是左值表达式
class StmtExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class ConstantInitExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static StmtExpr* Get(CompoundStmt* block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

 private:
  StmtExpr(CompoundStmt* block);

  CompoundStmt* block_;
};

class Stmt : public AstNode {};

class LabelStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static LabelStmt* Get(IdentifierExpr* label);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  explicit LabelStmt(IdentifierExpr* label);

  IdentifierExpr* ident_;
};

class CaseStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static CaseStmt* Get(std::int32_t case_value, Stmt* block);
  static CaseStmt* Get(std::int32_t case_value, std::int32_t case_value2,
                       Stmt* block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  CaseStmt(std::int32_t case_value, Stmt* block);
  CaseStmt(std::int32_t case_value, std::int32_t case_value2, Stmt* block);

  std::int32_t case_value_{};
  std::pair<std::int32_t, std::int32_t> case_value_range_;
  bool has_range_{false};

  Stmt* block_;
};

class DefaultStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static DefaultStmt* Get(Stmt* block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  DefaultStmt(Stmt* block);

  Stmt* block_;
};

class CompoundStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static CompoundStmt* Get();
  static CompoundStmt* Get(std::vector<Stmt*> stmts, Scope* scope);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  Scope* GetScope();
  std::vector<Stmt*> GetStmts();
  void AddStmt(Stmt* stmt);

 private:
  CompoundStmt() = default;
  explicit CompoundStmt(std::vector<Stmt*> stmts, Scope* scope);

  std::vector<Stmt*> stmts_;
  Scope* scope_;
};

class ExprStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static ExprStmt* Get(Expr* expr = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  Expr* GetExpr() const;

 private:
  explicit ExprStmt(Expr* expr = nullptr);

  Expr* expr_;
};

class IfStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static IfStmt* Get(Expr* cond, Stmt* then_block, Stmt* else_block = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  IfStmt(Expr* cond, Stmt* then_block, Stmt* else_block = nullptr);

  Expr* cond_;
  Stmt* then_block_;
  Stmt* else_block_;
};

class SwitchStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static SwitchStmt* Get(Expr* cond, Stmt* block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  SwitchStmt(Expr* cond, Stmt* block);

  Expr* cond_;
  Stmt* block_;
};

class WhileStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static WhileStmt* Get(Expr* cond, Stmt* block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  WhileStmt(Expr* cond, Stmt* block);

  Expr* cond_;
  Stmt* block_;
};

class DoWhileStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static DoWhileStmt* Get(Expr* cond, Stmt* block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  DoWhileStmt(Expr* cond, Stmt* block);

  Expr* cond_;
  Stmt* block_;
};

class ForStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static ForStmt* Get(Expr* init, Expr* cond, Expr* inc, Stmt* block,
                      Stmt* decl);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  ForStmt(Expr* init, Expr* cond, Expr* inc, Stmt* block, Stmt* decl);

  Expr *init_, *cond_, *inc_;
  Stmt* block_;
  Stmt* decl_;
};

class GotoStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static GotoStmt* Get(IdentifierExpr* ident);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  explicit GotoStmt(IdentifierExpr* ident);

  IdentifierExpr* ident_;
};

class ContinueStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static ContinueStmt* Get();

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
};

class BreakStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static BreakStmt* Get();

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
};

class ReturnStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static ReturnStmt* Get(Expr* expr = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  explicit ReturnStmt(Expr* expr = nullptr);

  Expr* expr_;
};

using ExtDecl = AstNode;

class TranslationUnit : public AstNode {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static TranslationUnit* Get();

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  void AddExtDecl(ExtDecl* ext_decl);
  std::vector<ExtDecl*> GetExtDecl() const;

 private:
  std::vector<ExtDecl*> ext_decls_;
};

class Initializer {
  friend class Declaration;
  friend class Initializers;
  friend class JsonGen;
  friend class CodeGen;

  friend bool operator<(const Initializer& lhs, const Initializer& rhs);

 public:
  Initializer(Type* type, std::int32_t offset, Expr* expr,
              const std::list<std::pair<Type*, std::int32_t>>& indexs);
  Type* GetType() const;
  int32_t GetOffset() const;
  Expr* GetExpr() const;
  std::list<std::pair<Type*, std::int32_t>> GetIndexs() const;

 private:
  Type* type_;
  std::int32_t offset_;
  Expr* expr_;

  std::list<std::pair<Type*, std::int32_t>> indexs_;
};

class Initializers {
  friend class JsonGen;
  friend class CodeGen;
  friend class Declaration;

 public:
  void AddInit(const Initializer& init);
  std::size_t size() const;

  auto begin() { return std::begin(inits_); }
  auto begin() const { return std::begin(inits_); }
  auto end() { return std::end(inits_); }
  auto end() const { return std::end(inits_); }

 private:
  std::vector<Initializer> inits_;
};

bool operator<(const Initializer& lhs, const Initializer& rhs);

class Declaration : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static Declaration* Get(IdentifierExpr* ident);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  bool HasLocalInit() const;
  void AddInits(const Initializers& inits);
  IdentifierExpr* GetIdent() const;
  bool IsObjDecl() const;
  void SetConstant(llvm::Constant* constant);
  llvm::Constant* GetGlobalInit() const;
  bool IsObjDeclInGlobal() const;
  ObjectExpr* GetObject() const;
  bool HasGlobalInit() const;
  std::vector<Initializer> GetLocalInits() const;

 private:
  explicit Declaration(IdentifierExpr* ident);

  IdentifierExpr* ident_;
  Initializers inits_;
  bool value_init_{false};

  llvm::Constant* constant_{};
};

class FuncDef : public ExtDecl {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static FuncDef* Get(IdentifierExpr* ident);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  void SetBody(CompoundStmt* body);
  std::string GetName() const;
  enum Linkage GetLinkage() const;
  QualType GetFuncType() const;
  IdentifierExpr* GetIdent() const;

 private:
  explicit FuncDef(IdentifierExpr* ident);

  IdentifierExpr* ident_;
  CompoundStmt* body_;
};

template <typename T, typename... Args>
T* MakeNode(const Location& loc, Args&&... args) {
  auto t{T::Get(std::forward<Args>(args)...)};
  t->SetLoc(loc);
  t->Check();
  return t;
}

}  // namespace kcc

#endif  // KCC_SRC_AST_H_
