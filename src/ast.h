//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_AST_H_
#define KCC_SRC_AST_H_

#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>

#include <QMetaEnum>
#include <QObject>
#include <QString>
#include <cassert>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "error.h"
#include "token.h"
#include "type.h"

namespace kcc {

class Declaration;
class Object;
class Enumerator;
class Visitor;

class AstNodeTypes : public QObject {
  Q_OBJECT
 public:
  enum Type {
    kCompoundStmt,
    kExprStmt,
    kIfStmt,
    kWhileStmt,
    kDoWhileStmt,
    kForStmt,
    kBreakStmt,
    kContinueStmt,
    kGotoStmt,
    kReturnStmt,
    kCaseStmt,
    kDefaultStmt,
    kSwitchStmt,
    kLabelStmt,

    kUnaryOpExpr,
    kBinaryOpExpr,
    kConditionOpExpr,
    kTypeCastExpr,
    kFunctionCallExpr,
    kConstantExpr,
    kIdentifier,
    kEnumerator,
    kObject,
    kTranslationUnit,
    kDeclaration,
    kFuncDef,
  };

  Q_ENUM(Type)

  static QString ToString(Type type) {
    return QMetaEnum::fromType<Type>().valueToKey(type) + 1;
  }
};

using AstNodeType = AstNodeTypes::Type;

class AstNode {
 public:
  virtual ~AstNode() = default;

  virtual AstNodeType Kind() const = 0;
  virtual void Accept(Visitor& visitor) const = 0;
  virtual void Check() = 0;

 protected:
  AstNode() = default;
};

using ExtDecl = AstNode;

class Expr : public AstNode {
 public:
  virtual bool IsLValue() const = 0;

  Token GetToken() const;
  void SetToken(const Token& tok);

  QualType GetQualType() const;
  const Type* GetType() const;
  Type* GetType();

  bool IsConst() const;
  bool IsRestrict() const;

  void EnsureCompatible(QualType lhs, QualType rhs) const;
  void EnsureCompatibleOrVoidPtr(QualType lhs, QualType rhs) const;
  // 数组函数隐式转换
  static Expr* MayCast(Expr* expr);
  static Expr* MayCastTo(Expr* expr, QualType to);

 protected:
  explicit Expr(const Token& tok, QualType type = {});

  Token tok_;
  QualType type_;
};

/*
 * =
 * + - * / % & | ^ << >>
 * && ||
 * == != < > <= >=
 * [] . ->
 * ,
 */
class BinaryOpExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static BinaryOpExpr* Get(const Token& tok, Tag tag, Expr* lhs, Expr* rhs);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;

  virtual void Check() override;

 private:
  BinaryOpExpr(const Token& tok, Tag tag, Expr* lhs, Expr* rhs);

  // 通常算术转换
  Type* Convert();

  void AssignOpCheck();
  void AddOpCheck();
  void SubOpCheck();
  void MultiOpCheck();
  void BitwiseOpCheck();
  void ShiftOpCheck();
  void LogicalOpCheck();
  void EqualityOpCheck();
  void RelationalOpCheck();
  void IndexOpCheck();
  void MemberRefOpCheck();
  void CommaOpCheck();

  Tag op_;
  Expr* lhs_;
  Expr* rhs_;
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
  friend class JsonGen;
  friend class CodeGen;

 public:
  static UnaryOpExpr* Get(const Token& tok, Tag tag, Expr* expr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;

  virtual void Check() override;

 private:
  UnaryOpExpr(const Token& tok, Tag tag, Expr* expr);

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
  friend class JsonGen;
  friend class CodeGen;

 public:
  static TypeCastExpr* Get(Expr* expr, QualType to);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

 private:
  TypeCastExpr(Expr* expr, QualType to);

  Expr* expr_;
  QualType to_;
};

class ConditionOpExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static ConditionOpExpr* Get(const Token& tok, Expr* cond, Expr* lhs,
                              Expr* rhs);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

 private:
  ConditionOpExpr(const Token& tok, Expr* cond, Expr* lhs, Expr* rhs)
      : Expr{tok},
        cond_(Expr::MayCast(cond)),
        lhs_(Expr::MayCast(lhs)),
        rhs_(Expr::MayCast(rhs)) {}

  // 通常算术转换
  Type* Convert();

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
  virtual bool IsLValue() const override;
  virtual void Check() override;

  Type* GetFuncType() const;

 private:
  explicit FuncCallExpr(Expr* callee, std::vector<Expr*> args = {});

  Expr* callee_;
  std::vector<Expr*> args_;
};

class Constant : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static Constant* Get(const Token& tok, std::int32_t val);
  static Constant* Get(const Token& tok, Type* type, std::uint64_t val);
  static Constant* Get(const Token& tok, Type* type, double val);
  static Constant* Get(const Token& tok, Type* type, const std::string& val);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

  long GetIntegerVal() const;
  double GetFloatPointVal() const;
  std::string GetStrVal() const;

 private:
  Constant(const Token& tok, std::int32_t val)
      : Expr(tok, QualType{ArithmeticType::Get(32)}), integer_val_(val) {}
  Constant(const Token& tok, Type* type, std::uint64_t val)
      : Expr(tok, type), integer_val_(val) {}
  Constant(const Token& tok, Type* type, double val)
      : Expr(tok, type), float_point_val_(val) {}
  Constant(const Token& tok, Type* type, const std::string& val)
      : Expr(tok, type), str_val_(val) {}

  std::uint64_t integer_val_{};
  double float_point_val_{};
  std::string str_val_;
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
class Identifier : public Expr {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static Identifier* Get(const Token& tok, QualType type, enum Linkage linkage,
                         bool is_type_name);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

  enum Linkage GetLinkage() const;
  void SetLinkage(enum Linkage linkage);
  std::string GetName() const;
  bool IsTypeName() const;
  bool IsObject() const;
  bool IsEnumerator() const;

 protected:
  Identifier(const Token& tok, QualType type, enum Linkage linkage,
             bool is_type_name);

  enum Linkage linkage_;
  bool is_type_name_;
};

class Enumerator : public Identifier {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  static Enumerator* Get(const Token& tok, std::int32_t val);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

  std::int32_t GetVal() const;

 private:
  Enumerator(const Token& tok, std::int32_t val);

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
class Object : public Identifier {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static Object* Get(const Token& tok, QualType type,
                     std::uint32_t storage_class_spec = 0,
                     enum Linkage linkage = kNone, bool anonymous = false,
                     bool in_global = false);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

  bool IsStatic() const;
  void SetStorageClassSpec(std::uint32_t storage_class_spec);
  std::int32_t GetAlign() const;
  void SetAlign(std::int32_t align);
  std::int32_t GetOffset() const;
  void SetOffset(std::int32_t offset);
  Declaration* GetDecl();
  void SetDecl(Declaration* decl);
  bool HasInit() const;
  bool Anonymous() const;

 private:
  Object(const Token& tok, QualType type, std::uint32_t storage_class_spec = 0,
         enum Linkage linkage = kNone, bool anonymous = false,
         bool in_global = false);

  bool anonymous_;
  std::uint32_t storage_class_spec_;
  std::int32_t align_{};
  std::int32_t offset_{};
  Declaration* decl_;

  bool in_global_{};
  llvm::AllocaInst* local_ptr_{};
  llvm::GlobalValue* global_ptr_{};
};

class Stmt : public AstNode {};

class LabelStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static LabelStmt* Get(Identifier* label);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  explicit LabelStmt(Identifier* label);

  Identifier* ident_;
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

 private:
  explicit ExprStmt(Expr* expr = nullptr);

  Expr* expr_;
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

class BreakStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static BreakStmt* Get();

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
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

class GotoStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static GotoStmt* Get(Identifier* ident);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  explicit GotoStmt(Identifier* ident) : ident_{ident} {}

  Identifier* ident_;
};

class Initializer {
  friend class JsonGen;
  friend class CodeGen;
  friend bool operator<(const Initializer& lhs, const Initializer& rhs);

 public:
  Initializer(Type* type, std::int32_t offset, Expr* expr)
      : type_(type), offset_(offset), expr_(expr) {}

 private:
  Type* type_;
  std::int32_t offset_;
  Expr* expr_;
};

bool operator<(const Initializer& lhs, const Initializer& rhs);

class Declaration : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static Declaration* Get(Identifier* ident);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  void AddInit(const Initializer& init);
  void AddInits(const std::set<Initializer>& inits) { inits_ = inits; }
  Identifier* GetIdent() const { return ident_; }
  bool HasInit() const;
  bool IsObj() const { return ident_->IsObject(); }

 private:
  explicit Declaration(Identifier* ident) : ident_{ident} {}

  Identifier* ident_;
  std::set<Initializer> inits_;
};

class TranslationUnit : public AstNode {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static TranslationUnit* Get();

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  void AddExtDecl(ExtDecl* ext_decl);

 private:
  std::vector<ExtDecl*> ext_decls_;
};

class FuncDef : public ExtDecl {
  friend class JsonGen;
  friend class CodeGen;

 public:
  static FuncDef* Get(Identifier* ident);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  void SetBody(CompoundStmt* body);
  std::string GetName() const;
  enum Linkage GetLinkage() const;
  QualType GetFuncType() const;
  Identifier* GetIdent() const;

 private:
  explicit FuncDef(Identifier* ident) : ident_(ident) {}

  Identifier* ident_;
  CompoundStmt* body_;
};

template <typename T, typename... Args>
T* MakeAstNode(Args&&... args) {
  auto t{T::Get(std::forward<Args>(args)...)};
  t->Check();
  return t;
}

}  // namespace kcc

#endif  // KCC_SRC_AST_H_
