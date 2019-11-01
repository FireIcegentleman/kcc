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

class AstNodeTypes : public QObject {
  Q_OBJECT
 public:
  enum Type {
    kAstNode,

    kStmt,
    kCompoundStmt,
    kExprStmt,
    kIfStmt,
    kWhileStmt,
    kDoWhileStmt,
    kForStmt,
    kBreakStmt,
    kContinueStmt,
    kReturnStmt,
    kCaseStmt,
    kDefaultStmt,
    kSwitchStmt,
    kLabelStmt,
    kGotoStmt,

    kExpr,
    kUnaryOpExpr,
    kBinaryOpExpr,
    kTernaryOpExpress,
    kTypeCastExpr,
    kSizeofExpr,
    kFunctionCallExpr,
    kVariableExpr,
    kArrayIndexExpr,
    kInitializationExpr,

    kDeclaration,
    kDeclarationList,
    kFunctionDeclaration,
    kFunctionDefinition,

    kInt32Constant,
    kUInt32Constant,
    kInt64Constant,
    kUInt64Constant,
    kFloatConstant,
    kDoubleConstant,
    kLongDoubleConstant,
    kStringLiteral,
  };

  Q_ENUM(Type)

  static QString ToString(Type type) {
    return QMetaEnum::fromType<Type>().valueToKey(type) + 1;
  }
};

class Visitor;

using AstNodeType = AstNodeTypes::Type;

class AstNode {
 public:
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual AstNodeType Kind() const = 0;
  virtual void Accept(Visitor& visitor) const = 0;
};

class Expr : public AstNode {
 public:
  void SetLoc(const SourceLocation& loc) { loc_ = loc; }
  SourceLocation GetLoc() const { return loc_; }
  virtual bool IsLValue() const = 0;
  virtual void TypeCheck() = 0;
  bool IsConstQualified() const { return type_->IsConstQualified(); }
  bool IsRestrictQualified() const { return type_->IsRestrictQualified(); }
  bool IsVolatileQualified() const { return type_->IsVolatileQualified(); }
  static std::shared_ptr<Expr> MayCast(std::shared_ptr<Expr> expr);
  static std::shared_ptr<Expr> CastTo(std::shared_ptr<Expr> expr,
                                      std::shared_ptr<Type> type);
  std::shared_ptr<Type> GetType() const { return type_; }

 protected:
  // 运算符/标识符的位置
  SourceLocation loc_;
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
 public:
  BinaryOpExpr(Tag op, std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs)
      : op_(op), lhs_(lhs), rhs_(rhs) {}
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override {
    // 在 C++ 中赋值运算符表达式是左值表达式，逗号运算符表达式可以是左值表达式
    // 而在 C 中两者都绝不是
    switch (op_) {
      case Tag::kLeftSquare:
        return true;
      case Tag::kPeriod:
        return lhs_->IsLValue();
      default:
        return false;
    }
  }

  virtual void TypeChecking();
  void SubScriptingOpTypeChecking();
  void MemberRefOpTypeChecking();
  void MultiOpTypeChecking();
  void AdditiveOpTypeChecking();
  void ShiftOpTypeChecking();
  void RelationalOpTypeChecking();
  void EqualityOpTypeChecking();
  void BitwiseOpTypeChecking();
  void LogicalOpTypeChecking();
  void AssignOpTypeChecking();
  void CommaOpTypeChecking();

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
 public:
  UnaryOpExpr(Tag op, std::shared_ptr<Expr> expr) : op_(op), expr_(expr) {}
  virtual void Accept(Visitor& visitor) const;

 private:
  Tag op_;
  std::shared_ptr<Expr> expr_;
};

class TypeCastExpr : public Expr {
 public:
  TypeCastExpr(std::shared_ptr<Expr> expr, std::shared_ptr<Type> to)
      : expr_{expr}, to_{to} {}

 private:
  std::shared_ptr<Expr> expr_;
  std::shared_ptr<Type> to_;
};

class ConditionOpExpr : public Expr {
 public:
  ConditionOpExpr(std::shared_ptr<Expr> cond, std::shared_ptr<Expr> true_expr,
                  std::shared_ptr<Expr> false_expr)
      : cond_(Expr::MayCast(cond)),
        true_expr_(Expr::MayCast(true_expr)),
        false_expr_(Expr::MayCast(false_expr)) {}
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override {
    // 和 C++ 不同，C 的条件运算符表达式绝不是左值
    return false;
  }
  virtual void TypeCheck() override {
    if (!cond_->GetType()->IsScalarTy()) {
      Error(loc_,
            "value of type '{}' is not contextually convertible to 'bool'",
            cond_->GetType()->Dump());
    }

    auto lhs_ty{true_expr_->GetType()};
    auto rhs_ty{false_expr_->GetType()};
    if (lhs_ty->IsArithmeticTy() && rhs_ty->IsArithmeticTy()) {
      auto type{Type::ArithmeticConversions(lhs_ty, rhs_ty)};
      if (lhs_ty->Equal(type)) {
        true_expr_ = std::make_shared<TypeCastExpr>(true_expr_, type);
      } else if (rhs_ty->Equal(type)) {
        true_expr_ = std::make_shared<TypeCastExpr>(true_expr_, type);
      }
      type_ = type;
    } else if (lhs_ty->IsStructTy() && rhs_ty->IsStructTy()) {
      if (lhs_ty->Equal(rhs_ty)) {
        type_ = lhs_ty;
      } else {
        Error(loc_, "incompatible operand types ('{}' and '{}')",
              lhs_ty->Dump(), rhs_ty->Dump());
      }
    } else if (lhs_ty->IsVoidTy() && rhs_ty->IsVoidTy()) {
      type_ = lhs_ty;
    } else if (lhs_ty->IsPointerTy() && rhs_ty->IsIntegerTy()) {
      // 一个是指针一个是空指针
    } else if (rhs_ty->IsPointerTy() && lhs_ty->IsIntegerTy()) {
    } else if (lhs_ty->IsPointerTy() && rhs_ty->IsPointerTy()) {
    } else {
      Error(loc_, "incompatible operand types ('{}' and '{}')", lhs_ty->Dump(),
            rhs_ty->Dump());
    }
  }

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Expr> true_expr_;
  std::shared_ptr<Expr> false_expr_;
};

class FuncCallExpr : public Expr {
 public:
  FuncCallExpr() = default;
  explicit FuncCallExpr(std::shared_ptr<Expr> callee,
                        std::vector<std::shared_ptr<Expr>> args = {})
      : callee_{callee}, args_{args} {}

  void AddArg(std::unique_ptr<Expr> arg) { args_.push_back(std::move(arg)); }

  AstNodeType Kind() const override { return AstNodeType::kFunctionCallExpr; }

  void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}

 private:
  std::shared_ptr<Expr> callee_;
  std::vector<std::shared_ptr<Expr>> args_;
};

class Int32Constant : public Expr {
 public:
  Int32Constant() = default;
  explicit Int32Constant(std::int32_t value) : value_{value} {}

  AstNodeType Kind() const override { return AstNodeType::kInt32Constant; }
  void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}

 private:
  std::int32_t value_{};
};

class UInt32Constant : public Expr {
 public:
  UInt32Constant() = default;
  explicit UInt32Constant(std::uint32_t value) : value_{value} {}

  AstNodeType Kind() const override { return AstNodeType::kUInt32Constant; }
  void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}

 private:
  std::uint32_t value_{};
};

class Int64Constant : public Expr {
 public:
  Int64Constant() = default;
  explicit Int64Constant(std::int64_t value) : value_{value} {}

  AstNodeType Kind() const override { return AstNodeType::kInt64Constant; }
  void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}

 private:
  std::int64_t value_{};
};

class UInt64Constant : public Expr {
 public:
  UInt64Constant() = default;
  explicit UInt64Constant(std::uint64_t value) : value_{value} {}

  AstNodeType Kind() const override { return AstNodeType::kUInt64Constant; }
  void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}

 private:
  std::uint64_t value_{};
};

class FloatConstant : public Expr {
 public:
  FloatConstant() = default;
  explicit FloatConstant(float value) : value_{value} {}

  AstNodeType Kind() const override { return AstNodeType::kFloatConstant; }
  void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}

 private:
  float value_{};
};

class DoubleConstant : public Expr {
 public:
  DoubleConstant() = default;
  explicit DoubleConstant(double value) : value_{value} {}

  AstNodeType Kind() const override { return AstNodeType::kDoubleConstant; }
  void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}

 private:
  double value_{};
};

class LongDoubleConstant : public Expr {
 public:
  LongDoubleConstant() = default;
  explicit LongDoubleConstant(long double value) : value_{value} {}

  AstNodeType Kind() const override { return AstNodeType::kLongDoubleConstant; }
  void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}

 private:
  long double value_{};
};

class StringLiteral : public Expr {
 public:
  StringLiteral() = default;
  explicit StringLiteral(std::string value) : value_{std::move(value)} {}

  AstNodeType Kind() const override { return AstNodeType::kStringLiteral; }
  void Accept(Visitor& visitor) const override;

  std::string value_;
};

class Stmt : public AstNode {};

class LabelStmt : public Stmt {
 public:
  LabelStmt() = default;
  explicit LabelStmt(const std::string& label) : label_{label} {}

  AstNodeType Kind() const override { return AstNodeType::kLabelStmt; }
  void Accept(Visitor& visitor) const override;

 private:
  std::string label_;
};

class IfStmt : public Stmt {
 public:
  IfStmt() = default;
  IfStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> then_block,
         std::shared_ptr<Stmt> else_block)
      : cond_{cond}, then_block_{then_block}, else_block_{else_block} {}

  AstNodeType Kind() const override { return AstNodeType::kIfStmt; }
  void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Stmt> then_block_;
  std::shared_ptr<Stmt> else_block_;
};

// break / continue / goto
class JumpStmt : public Stmt {
 public:
  JumpStmt() = default;
  explicit JumpStmt(std::shared_ptr<LabelStmt> label) : label_{label} {}

  AstNodeType Kind() const override { return AstNodeType::kIfStmt; }
  void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<LabelStmt> label_;
};

class ReturnStmt : public Stmt {
 public:
  ReturnStmt() = default;
  explicit ReturnStmt(std::shared_ptr<Expr> expr) : expr_{expr} {}

  AstNodeType Kind() const override { return AstNodeType::kReturnStmt; }
  void Accept(Visitor& visitor) const override;

 private:
  std::shared_ptr<Expr> expr_;
};

class CompoundStmt : public Stmt {
 public:
  CompoundStmt() = default;
  explicit CompoundStmt(std::vector<std::shared_ptr<Stmt>> stmts)
      : stmts_{stmts} {}

  void AddStmt(std::shared_ptr<Stmt> statement) { stmts_.push_back(statement); }

  AstNodeType Kind() const override { return AstNodeType::kCompoundStmt; }
  void Accept(Visitor& visitor) const override;

 private:
  std::vector<std::shared_ptr<Stmt>> stmts_;
  // TODO scope
};

enum StorageClassSpec {
  kTypedef = 0x1,
  kExtern = 0x2,
  kStatic = 0x4,
  // 不支持
  kThreadLocal = 0x8,
  kAuto = 0x10,
  kRegister = 0x20
};

enum TypeSpec {
  kVoid = 0x1,
  kChar = 0x2,
  kShort = 0x4,
  kInt = 0x8,
  kLong = 0x10,
  kFloat = 0x20,
  kDouble = 0x40,
  kSigned = 0x80,
  kUnsigned = 0x100,
  kBool = 0x200,
  // 不支持
  kComplex = 0x400,
  // 不支持
  kAtomicTypeSpec = 0x800,
  kStructUnionSpec = 0x1000,
  kEnumSpec = 0x2000,
  kTypedefName = 0x4000,

  kLongLong = 0x8000
};

enum TypeSpecCompatibility {
  kCompSigned = kShort | kInt | kLong | kLongLong,
  kCompUnsigned = kShort | kInt | kLong | kLongLong,
  kCompChar = kSigned | kUnsigned,
  kCompShort = kSigned | kUnsigned | kInt,
  kCompInt = kSigned | kUnsigned | kShort | kLong | kLongLong,
  kCompLong = kSigned | kUnsigned | kInt | kLong,
  kCompDouble = kLong
};

enum TypeQualifiers {
  kConst = 0x1,
  kRestrict = 0x2,
  // 不支持
  kVolatile = 0x4,
  // 不支持
  kAtomic = 0x8
};

enum FuncSpec { kInline = 0x1, kNoreturn = 0x2 };

class TranslationUnit : public AstNode {
 public:
  virtual void Accept(Visitor& visitor) const override;
  void AddStmt(std::shared_ptr<Stmt> stmt) { stmts_.push_back(stmt); }

 private:
  std::vector<std::shared_ptr<Stmt>> stmts_;
};

// 标识符能指代下列类型的实体：
// 对象
// 函数
// struct / union / enumerations
// 结构体或联合体成员
// 枚举常量
// typedef 名
// 标号名
// 宏名
// 宏形参名
// 宏名或宏形参名以外的每个标识符都拥有作用域，并且可以拥有链接
class Identifier : public Expr {
 public:
  enum Linkage { kNone, kInternal, kExternal };
  explicit Identifier(const std::string& name, enum Linkage linkage)
      : name_{name}, linkage_{linkage} {}
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return false; }
  virtual void TypeCheck() override {}
  enum Linkage GetLinkage() const { return linkage_; }
  void SetLinkage(enum Linkage linkage) { linkage_ = linkage; }

 private:
  std::string name_;
  enum Linkage linkage_ { kNone };
};

// 每个对象拥有
// 大小（可由 sizeof 确定）
// 对齐要求（可由 _Alignof 确定） (C11 起)
// 存储期（自动、静态、分配、线程局域）
// 生存期（等于存储期或临时）
// 有效类型（见下）
// 值（可以是不确定的）
// 可选项，表示该对象的标识符
class Object : public Identifier {
 public:
  virtual void Accept(Visitor& visitor) const override;
  virtual bool IsLValue() const override { return true; }
  virtual void TypeCheck() override {}

 private:
};

struct Initializer {
  Initializer(Type* type, int offset, Expr* expr,
              unsigned char bitFieldBegin = 0, unsigned char bitFieldWidth = 0)
      : type_(type),
        offset_(offset),
        bitFieldBegin_(bitFieldBegin),
        bitFieldWidth_(bitFieldWidth),
        expr_(expr) {}

  bool operator<(const Initializer& rhs) const;

  Type* type_;
  int offset_;
  unsigned char bitFieldBegin_;
  unsigned char bitFieldWidth_;

  Expr* expr_;
};

class Decl : public Stmt {
 public:
  explicit Decl(std::shared_ptr<Object> object) : object_{object} {}
  virtual void Accept(Visitor& visitor) const override;
  void AddInit(const Initializer& init);
  void AddInits(const std::set<Initializer>& inits);

 private:
  std::shared_ptr<Object> object_;
  std::set<Initializer> inits_;
};

class Enumerator : public Identifier {
 public:
 private:
};

}  // namespace kcc

#endif  // KCC_SRC_AST_H_
