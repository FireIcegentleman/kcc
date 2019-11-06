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
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual AstNodeType Kind() const = 0;
  virtual void Accept(Visitor& visitor) const = 0;
  virtual void Check() = 0;
};

using ExtDecl = AstNode;

class Expr : public AstNode {
 public:
  explicit Expr(const Token& tok, QualType type = {});

  virtual bool IsLValue() const = 0;

  Token GetToken() const;
  void SetToken(const Token& tok);

  QualType GetQualType() const;
  std::shared_ptr<Type> GetType() const;

  bool IsConst() const;
  bool IsRestrict() const;

  void EnsureCompatible(QualType lhs, QualType rhs) const;
  void EnsureCompatibleOrVoidPtr(QualType lhs, QualType rhs) const;
  // 数组函数隐式转换
  static std::shared_ptr<Expr> MayCast(const std::shared_ptr<Expr>& expr);
  static std::shared_ptr<Expr> MayCastTo(std::shared_ptr<Expr> expr,
                                         QualType to);

 protected:
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
  BinaryOpExpr(const Token& tok, Tag tag, std::shared_ptr<Expr> lhs,
               std::shared_ptr<Expr> rhs);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;

  virtual void Check() override;

 private:
  // 通常算术转换
  std::shared_ptr<Type> Convert();

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
  UnaryOpExpr(const Token& tok, Tag tag, std::shared_ptr<Expr> expr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;

  virtual void Check() override;

 private:
  void IncDecOpCheck();
  void UnaryAddSubOpCheck();
  void NotOpCheck();
  void LogicNotOpCheck();
  void DerefOpCheck();
  void AddrOpCheck();

  Tag op_;
  std::shared_ptr<Expr> expr_;
};

class TypeCastExpr : public Expr {
  template <typename T>
  friend class CalcExpr;
  friend class JsonGen;
  friend class CodeGen;

 public:
  TypeCastExpr(std::shared_ptr<Expr> expr, std::shared_ptr<Type> to);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Expr> expr_;
  QualType to_;
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
  virtual void Check() override;

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
                        std::vector<std::shared_ptr<Expr>> args = {});

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

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
      : Expr(tok, QualType{ArithmeticType::Get(32)}), integer_val_(val) {}
  Constant(const Token& tok, std::shared_ptr<Type> type, std::uint64_t val)
      : Expr(tok, type), integer_val_(val) {}
  Constant(const Token& tok, std::shared_ptr<Type> type, double val)
      : Expr(tok, type), float_point_val_(val) {}
  Constant(const Token& tok, std::shared_ptr<Type> type, const std::string& val)
      : Expr(tok, type), str_val_(val) {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

  long GetIntegerVal() const;
  double GetFloatPointVal() const;
  std::string GetStrVal() const;

 private:
  std::uint64_t integer_val_;
  double float_point_val_;
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
  Identifier(const Token& tok, QualType type, enum Linkage linkage,
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
  virtual void Check() override;

  std::int32_t GetVal() const;

 private:
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
  Object(const Token& tok, QualType type, std::uint32_t storage_class_spec,
         enum Linkage linkage = kNone, bool anonymous = false);

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
  std::shared_ptr<Declaration> GetDecl();
  void SetDecl(std::shared_ptr<Declaration> decl);
  bool HasInit() const;
  bool Anonymous() const;

 private:
  bool anonymous_;
  std::uint32_t storage_class_spec_;
  std::int32_t align_{};
  std::int32_t offset_{};
  std::shared_ptr<Declaration> decl_;
};

class Stmt : public AstNode {};

class LabelStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit LabelStmt(std::shared_ptr<Identifier> label);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Identifier> ident_;
};

class IfStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  IfStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> then_block,
         std::shared_ptr<Stmt> else_block = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Stmt> then_block_;
  std::shared_ptr<Stmt> else_block_;
};

class ReturnStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit ReturnStmt(std::shared_ptr<Expr> expr = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Expr> expr_;
};

class CompoundStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit CompoundStmt(std::vector<std::shared_ptr<Stmt>> stmts,
                        std::shared_ptr<Scope> scope);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

  std::shared_ptr<Scope> GetScope();
  std::vector<std::shared_ptr<Stmt>> GetStmts();

 private:
  std::vector<std::shared_ptr<Stmt>> stmts_;
  std::shared_ptr<Scope> scope_;
};

class ExprStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit ExprStmt(std::shared_ptr<Expr> expr = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Expr> expr_;
};

class WhileStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  WhileStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Stmt> block_;
};

class DoWhileStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  DoWhileStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Stmt> block_;
};

class ForStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  ForStmt(std::shared_ptr<Expr> init, std::shared_ptr<Expr> cond,
          std::shared_ptr<Expr> inc, std::shared_ptr<Stmt> block,
          std::shared_ptr<Stmt> decl);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Expr> init_, cond_, inc_;
  std::shared_ptr<Stmt> block_;
  std::shared_ptr<Stmt> decl_;
};

class CaseStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit CaseStmt(std::int32_t case_value, std::shared_ptr<Stmt> block);
  CaseStmt(std::int32_t case_value, std::int32_t case_value2,
           std::shared_ptr<Stmt> block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::int32_t case_value_{};
  std::pair<std::int32_t, std::int32_t> case_value_range_;
  bool has_range_{false};

  std::shared_ptr<Stmt> block_;
};

class DefaultStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  DefaultStmt(std::shared_ptr<Stmt> block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Stmt> block_;
};

class SwitchStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit SwitchStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Stmt> block_;
};

class BreakStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
};

class ContinueStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;
};

class GotoStmt : public Stmt {
  friend class JsonGen;
  friend class CodeGen;

 public:
  explicit GotoStmt(std::shared_ptr<Identifier> ident) : ident_{ident} {}

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor& visitor) const override;
  virtual void Check() override;

 private:
  std::shared_ptr<Identifier> ident_;
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
  virtual void Check() override;

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
  virtual void Check() override;

  void AddExtDecl(std::shared_ptr<ExtDecl> ext_decl);

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
  virtual void Check() override;

  void SetBody(std::shared_ptr<CompoundStmt> body);
  std::string GetName() const;
  enum Linkage GetLinkage() const;
  QualType GetFuncType() const;

 private:
  std::shared_ptr<Identifier> ident_;
  std::shared_ptr<CompoundStmt> body_;
};

template <typename T, typename... Args>
std::shared_ptr<T> MakeAstNode(Args&&... args) {
  auto t{std::make_shared<T>(std::forward<Args>(args)...)};
  t->Check();
  return t;
}

}  // namespace kcc

#endif  // KCC_SRC_AST_H_
