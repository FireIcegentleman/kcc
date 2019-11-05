//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_AST_H_
#define KCC_SRC_AST_H_

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
    kJumpStmt,
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
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual AstNodeType Kind() const = 0;
  virtual void Accept(Visitor& visitor) const = 0;
};

using ExtDecl = AstNode;

class Expr : public AstNode {
 public:
  explicit Expr(const Token& tok, std::shared_ptr<Type> type = nullptr);

  virtual bool IsLValue() const = 0;
  virtual void TypeCheck() = 0;

  Token GetToken() const;
  void SetToken(const Token& tok);
  std::shared_ptr<Type> GetType() const;
  bool IsConstQualified() const;
  bool IsRestrictQualified() const;
  bool IsVolatileQualified() const;
  void EnsureCompatible(const std::shared_ptr<Type>& lhs,
                        const std::shared_ptr<Type>& rhs);
  void EnsureCompatibleOrVoidPtr(const std::shared_ptr<Type>& lhs,
                                 const std::shared_ptr<Type>& rhs);
  static std::shared_ptr<Expr> MayCast(std::shared_ptr<Expr> expr);
  static std::shared_ptr<Expr> CastTo(std::shared_ptr<Expr> expr,
                                      std::shared_ptr<Type> type);

 protected:
  Token tok_;
  std::shared_ptr<Type> type_;
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
  BinaryOpExpr(const Token& tok, std::shared_ptr<Expr> lhs,
               std::shared_ptr<Expr> rhs);
  BinaryOpExpr(const Token& tok, Tag tag, std::shared_ptr<Expr> lhs,
               std::shared_ptr<Expr> rhs);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  // 通常算术转换
  std::shared_ptr<Type> Convert();

  virtual void TypeCheck() override;
  void IndexOpTypeCheck();
  void AssignOpTypeCheck();
  void AddOpTypeCheck();
  void SubOpTypeCheck();
  void MultiOpTypeCheck();
  void BitwiseOpTypeCheck();
  void ShiftOpTypeCheck();
  void LogicalOpTypeCheck();
  void EqualityOpTypeCheck();
  void RelationalOpTypeCheck();
  void MemberRefOpTypeCheck();
  void CommaOpTypeCheck();

 private:
  Tag op_;
  std::shared_ptr<Expr> lhs_;
  std::shared_ptr<Expr> rhs_;
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
  UnaryOpExpr(const Token& tok, std::shared_ptr<Expr> expr);
  UnaryOpExpr(const Token& tok, Tag tag, std::shared_ptr<Expr> expr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;

  virtual void TypeCheck() override;
  void IncDecOpTypeCheck();
  void AddrOpTypeCheck();
  void DerefOpTypeCheck();
  void UnaryAddSubOpTypeCheck();
  void NotOpTypeCheck();
  void LogicNotOpTypeCheck();

 private:
  Tag op_;
  std::shared_ptr<Expr> expr_;
};

class TypeCastExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  TypeCastExpr(std::shared_ptr<Expr> expr, std::shared_ptr<Type> to)
      : Expr(expr->GetToken()), expr_{expr}, to_{to} {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void TypeCheck() override;

 private:
  std::shared_ptr<Expr> expr_;
  std::shared_ptr<Type> to_;
};

class ConditionOpExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  ConditionOpExpr(const Token& tok, std::shared_ptr<Expr> cond,
                  std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs)
      : Expr{tok},
        cond_(Expr::MayCast(cond)),
        lhs_(Expr::MayCast(lhs)),
        rhs_(Expr::MayCast(rhs)) {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void TypeCheck() override;

  // 通常算术转换
  std::shared_ptr<Type> Convert();

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Expr> lhs_;
  std::shared_ptr<Expr> rhs_;
};

class FuncCallExpr : public Expr {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit FuncCallExpr(std::shared_ptr<Expr> callee,
                        std::vector<std::shared_ptr<Expr>> args = {})
      : Expr{callee->GetToken()}, callee_{callee}, args_{std::move(args)} {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void TypeCheck() override;

  std::shared_ptr<Type> GetFuncType() const;

 private:
  std::shared_ptr<Expr> callee_;
  std::vector<std::shared_ptr<Expr>> args_;
};

class Constant : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  Constant(const Token& tok, std::int32_t val)
      : Expr(tok, IntegerType::Get(32)), int_val_(val) {}
  Constant(const Token& tok, std::shared_ptr<Type> type, std::uint64_t val)
      : Expr(tok, type), int_val_(val) {}
  Constant(const Token& tok, std::shared_ptr<Type> type, double val)
      : Expr(tok, type), float_val_(val) {}
  Constant(const Token& tok, std::shared_ptr<Type> type, const std::string& val)
      : Expr(tok, type), str_val_(val) {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void TypeCheck() override;

  long GetIntVal() const;
  double GetFloatVal() const;
  std::string GetStrVal() const;

 private:
  std::uint64_t int_val_;
  double float_val_;
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
  Identifier(const Token& tok, std::shared_ptr<Type> type, enum Linkage linkage,
             bool is_type_name)
      : Expr{tok, type}, linkage_{linkage}, is_type_name_{is_type_name} {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void TypeCheck() override;

  enum Linkage GetLinkage() const;
  void SetLinkage(enum Linkage linkage);
  std::string GetName() const;
  bool IsTypeName() const;
  bool IsObject() const;
  bool IsEnumerator() const;

 private:
  enum Linkage linkage_;
  bool is_type_name_;
};

class Enumerator : public Identifier {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  Enumerator(const Token& tok, std::int32_t val);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void TypeCheck() override;
  std::int32_t GetVal() const;

 private:
  std::shared_ptr<Constant> val_;
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
  Object(const Token& tok, std::shared_ptr<Type> type,
         enum Linkage linkage = kNone, bool anonymous = false)
      : Identifier{tok, type, linkage, false}, anonymous_{anonymous} {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void TypeCheck() override;

  bool IsStatic() const;
  std::int32_t GetAlign() const;
  std::int32_t GetOffset() const;
  void SetOffset(std::int32_t offset);
  std::shared_ptr<Declaration> GetDecl();
  void SetDecl(std::shared_ptr<Declaration> decl);
  bool HasInit() const;
  bool Anonymous() const;

 private:
  bool anonymous_;
  std::int32_t offset_;
  std::shared_ptr<Declaration> decl_;
};

class Stmt : public AstNode {};

class LabelStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit LabelStmt(const std::string& label);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::string label_;
};

class IfStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  IfStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> then_block,
         std::shared_ptr<Stmt> else_block = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Stmt> then_block_;
  std::shared_ptr<Stmt> else_block_;
};

// break / continue / goto
class JumpStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit JumpStmt(std::shared_ptr<LabelStmt> label);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<LabelStmt> label_;
};

class ReturnStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit ReturnStmt(std::shared_ptr<Expr> expr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> expr_;
};

class CompoundStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  CompoundStmt() = default;
  explicit CompoundStmt(std::vector<std::shared_ptr<Stmt>> stmts);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

  std::shared_ptr<Scope> GetScope();
  void AddStmt(std::shared_ptr<Stmt> stmt);

 private:
  std::vector<std::shared_ptr<Stmt>> stmts_;
  std::shared_ptr<Scope> scope_;
};

class ExprStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit ExprStmt(std::shared_ptr<Expr> expr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> expr_;
};

class WhileStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  WhileStmt(std::shared_ptr<Expr> cond, std::shared_ptr<CompoundStmt> block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<CompoundStmt> block_;
};

class DoWhileStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  DoWhileStmt(std::shared_ptr<Expr> cond, std::shared_ptr<CompoundStmt> block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<CompoundStmt> block_;
};

class ForStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  ForStmt(std::shared_ptr<Expr> init, std::shared_ptr<Expr> cond,
          std::shared_ptr<Expr> inc, std::shared_ptr<CompoundStmt> block,
          std::shared_ptr<Declaration> decl);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> init_, cond_, inc_;
  std::shared_ptr<CompoundStmt> block_;
  std::shared_ptr<Declaration> decl_;
};

class CaseStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit CaseStmt(std::int32_t case_value);
  CaseStmt(std::int32_t case_value, std::int32_t case_value2);

  void AddStmt(std::shared_ptr<Stmt> stmt);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::int32_t case_value_{};
  // GCC 扩展
  std::pair<std::int32_t, std::int32_t> case_value_range_;
  bool has_range_{false};

  std::shared_ptr<CompoundStmt> block_;
};

class DefaultStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  DefaultStmt() = default;

  void AddStmt(std::shared_ptr<Stmt> stmt);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<CompoundStmt> block_;
};

class SwitchStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit SwitchStmt(std::shared_ptr<Expr> choose);

  void AddCase(std::shared_ptr<CaseStmt> case_stmt);
  void SetDefault(std::shared_ptr<DefaultStmt> default_stmt);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> choose_;
  std::vector<std::shared_ptr<CaseStmt>> case_stmts_;
  std::shared_ptr<DefaultStmt> default_stmt_;
};

class Initializer {
  friend class JsonGen;
  friend class CodeGen;
  friend bool operator<(const Initializer& lhs, const Initializer& rhs);

 public:
  Initializer(std::shared_ptr<Type> type, std::int32_t offset,
              std::shared_ptr<Expr> expr)
      : type_(type), offset_(offset), expr_(expr) {}

 private:
  std::shared_ptr<Type> type_;
  std::int32_t offset_;
  std::shared_ptr<Expr> expr_;
};

bool operator<(const Initializer& lhs, const Initializer& rhs);

class Declaration : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit Declaration(std::shared_ptr<Identifier> ident) : ident_{ident} {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

  void AddInit(const Initializer& init);
  void AddInits(const std::set<Initializer>& inits) { inits_ = inits; }
  std::shared_ptr<Identifier> GetIdent() const { return ident_; }
  bool HasInit() const;
  bool IsObj() const { return ident_->IsObject(); }

 private:
  std::shared_ptr<Identifier> ident_;
  std::set<Initializer> inits_;
};

class TranslationUnit : public AstNode {
  friend class JsonGen;
  friend class CodeGen;

 public:
  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

  void AddStmt(std::shared_ptr<ExtDecl> ext_decl) {
    ext_decls_.push_back(ext_decl);
  }

 private:
  std::vector<std::shared_ptr<ExtDecl>> ext_decls_;
};

class FuncDef : public ExtDecl {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit FuncDef(std::shared_ptr<Identifier> ident) : ident_(ident) {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;

  void SetBody(std::shared_ptr<CompoundStmt> body);
  std::string GetName() const;
  enum Linkage GetLinkage();
  std::shared_ptr<Type> GetFuncType() const;

 private:
  std::shared_ptr<Identifier> ident_;
  std::shared_ptr<CompoundStmt> body_;
};

}  // namespace kcc

#endif  // KCC_SRC_AST_H_
