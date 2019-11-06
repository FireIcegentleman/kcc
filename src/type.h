//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_TYPE_H_
#define KCC_SRC_TYPE_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace kcc {

enum TypeSpec {
  kSigned = 0x1,
  kUnsigned = 0x2,
  kVoid = 0x4,
  kChar = 0x8,
  kShort = 0x10,
  kInt = 0x20,
  kLong = 0x40,
  kFloat = 0x80,
  kDouble = 0x100,
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

enum TypeQualifier {
  kConst = 0x1,
  kRestrict = 0x2,
  // 不支持
  kVolatile = 0x4,
  // 不支持
  kAtomic = 0x8
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

enum FuncSpec { kInline = 0x1, kNoreturn = 0x2 };

enum TypeSpecCompatibility {
  kCompSigned = kShort | kInt | kLong | kLongLong,
  kCompUnsigned = kShort | kInt | kLong | kLongLong,
  kCompChar = kSigned | kUnsigned,
  kCompShort = kSigned | kUnsigned | kInt,
  kCompInt = kSigned | kUnsigned | kShort | kLong | kLongLong,
  kCompLong = kSigned | kUnsigned | kInt | kLong,
  kCompDouble = kLong
};

class Type;
class PointerType;
class Object;
class Scope;

class QualType {
  friend bool operator==(QualType lhs, QualType rhs);
  friend bool operator!=(QualType lhs, QualType rhs);

 public:
  QualType() = default;
  QualType(std::shared_ptr<Type> type, std::uint32_t type_qual = 0)
      : type_{type}, type_qual_{type_qual} {}

  Type& operator*();
  const Type& operator*() const;
  std::shared_ptr<Type> operator->();
  const std::shared_ptr<Type> operator->() const;

  std::shared_ptr<Type> GetType();
  const std::shared_ptr<Type> GetType() const;
  std::uint32_t GetTypeQual() const;

  bool IsConst() const;
  bool IsRestrict() const;

 private:
  std::shared_ptr<Type> type_;
  std::uint32_t type_qual_;
};

bool operator==(QualType lhs, QualType rhs);
bool operator!=(QualType lhs, QualType rhs);

class Type : public std::enable_shared_from_this<Type> {
 public:
  // 数组函数隐式转换为指针
  // TODO 是否应该创建一个 ast 节点？
  static QualType MayCast(QualType type, bool in_proto = false);

  virtual ~Type() = default;

  // 位数而不是字节数
  virtual std::int32_t GetWidth() const = 0;
  virtual std::int32_t GetAlign() const = 0;
  virtual std::string ToString() const = 0;
  // 这里忽略了 cvr
  // 若涉及同一对象或函数的二个声明不使用兼容类型，则程序的行为未定义。
  virtual bool Compatible(const std::shared_ptr<Type>& other) const = 0;
  virtual bool Equal(const std::shared_ptr<Type>& other) const = 0;

  bool IsComplete() const;
  void SetComplete(bool complete) const;

  bool IsUnsigned() const;
  bool IsVoidTy() const;
  bool IsBoolTy() const;
  bool IsShortTy() const;
  bool IsIntTy() const;
  bool IsLongTy() const;
  bool IsLongLongTy() const;
  bool IsFloatTy() const;
  bool IsDoubleTy() const;
  bool IsLongDoubleTy() const;
  bool IsComplexTy() const;
  bool IsTypeName() const;

  bool IsPointerTy() const;
  bool IsArrayTy() const;
  bool IsStructTy() const;
  bool IsUnionTy() const;
  bool IsFunctionTy() const;

  bool IsObjectTy() const;
  bool IsCharacterTy() const;
  bool IsIntegerTy() const;
  bool IsRealTy() const;
  bool IsArithmeticTy() const;
  bool IsScalarTy() const;
  bool IsAggregateTy() const;
  bool IsDerivedTy() const;

  bool IsRealFloatPointTy() const;
  bool IsFloatPointTy() const;

  std::shared_ptr<PointerType> GetPointerTo();

  std::int32_t ArithmeticRank() const;
  std::uint64_t ArithmeticMaxIntegerValue() const;
  void ArithmeticSetUnsigned();

  QualType PointerGetElementType() const;

  void ArraySetNumElements(std::size_t num_elements);
  std::size_t ArrayGetNumElements() const;
  QualType ArrayGetElementType() const;

  bool StructHasName() const;
  void StructSetName(const std::string& name);
  std::string StructGetName() const;
  std::int32_t StructGetNumMembers() const;
  std::vector<std::shared_ptr<Object>> StructGetMembers();
  std::shared_ptr<Object> StructGetMember(const std::string& name) const;
  QualType StructGetMemberType(std::int32_t i) const;
  std::shared_ptr<Scope> StructGetScope();
  void StructAddMember(std::shared_ptr<Object> member);
  void StructMergeAnonymous(std::shared_ptr<Object> anonymous);
  std::int32_t StructGetOffset() const;

  bool FuncIsVarArgs() const;
  QualType FuncGetReturnType() const;
  std::int32_t FuncGetNumParams() const;
  QualType FuncGetParamType(std::int32_t i) const;
  std::vector<std::shared_ptr<Object>> FuncGetParams() const;
  void FuncSetFuncSpec(std::uint32_t func_spec);
  bool FuncIsInline() const;
  bool FuncIsNoreturn() const;

 protected:
  explicit Type(bool complete);

 private:
  mutable bool complete_{false};
};

class VoidType : public Type {
 public:
  static std::shared_ptr<VoidType> Get();

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual std::string ToString() const override;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& other) const override;

 private:
  VoidType();
};

class ArithmeticType : public Type {
  friend class Type;

 public:
  static std::shared_ptr<ArithmeticType> Get(std::uint32_t type_spec);

  static std::shared_ptr<Type> IntegerPromote(std::shared_ptr<Type> type);
  static std::shared_ptr<Type> MaxType(std::shared_ptr<Type> lhs,
                                       std::shared_ptr<Type> rhs);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual std::string ToString() const override;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& other) const override;

  std::int32_t Rank() const;
  std::uint64_t MaxIntegerValue() const;
  void SetUnsigned();

 private:
  explicit ArithmeticType(std::uint32_t type_spec);

  std::uint32_t type_spec_{};
};

class PointerType : public Type {
 public:
  static std::shared_ptr<PointerType> Get(QualType element_type);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual std::string ToString() const override;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& type) const override;

  QualType GetElementType() const;

 private:
  explicit PointerType(QualType element_type);

  QualType element_type_;
};

class ArrayType : public Type {
 public:
  static std::shared_ptr<ArrayType> Get(QualType contained_type,
                                        std::size_t num_elements = 0);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual std::string ToString() const override;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& other) const override;

  void SetNumElements(std::size_t num_elements);
  std::size_t GetNumElements() const;
  QualType GetElementType() const;

 private:
  ArrayType(QualType contained_type, std::size_t num_elements);

  QualType contained_type_;
  std::size_t num_elements_;
};

class StructType : public Type {
  friend class Type;

 public:
  static std::shared_ptr<StructType> Get(bool is_struct, bool has_name,
                                         std::shared_ptr<Scope> parent);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual std::string ToString() const override;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& other) const override;

  bool IsStruct() const;
  bool HasName() const;
  void SetName(const std::string& name);
  std::string GetName() const;

  std::int32_t GetNumMembers() const;
  std::vector<std::shared_ptr<Object>> GetMembers();
  std::shared_ptr<Object> GetMember(const std::string& name) const;
  QualType GetMemberType(std::int32_t i) const;
  std::shared_ptr<Scope> GetScope();
  std::int32_t GetOffset() const;

  void AddMember(std::shared_ptr<Object> member);
  void MergeAnonymous(std::shared_ptr<Object> anonymous);

 private:
  StructType(bool is_struct, bool has_name, std::shared_ptr<Scope> parent);

  // 计算新成员的开始位置
  static std::int32_t MakeAlign(std::int32_t offset, std::int32_t align);

  bool is_struct_{};
  bool has_name_{};

  std::string name_;
  std::vector<std::shared_ptr<Object>> members_;
  std::shared_ptr<Scope> scope_;

  std::int32_t offset_{};
  std::int32_t width_{};
  std::int32_t align_{};
};

class FunctionType : public Type {
 public:
  static std::shared_ptr<FunctionType> Get(
      QualType return_type, std::vector<std::shared_ptr<Object>> params,
      bool is_var_args = false);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual std::string ToString() const override;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& other) const override;

  bool IsVarArgs() const;
  QualType GetReturnType() const;
  std::int32_t GetNumParams() const;
  QualType GetParamType(std::int32_t i) const;
  std::vector<std::shared_ptr<Object>> GetParams() const;
  void SetFuncSpec(std::uint32_t func_spec);
  bool IsInline() const;
  bool IsNoreturn() const;

 private:
  FunctionType(QualType return_type, std::vector<std::shared_ptr<Object>> param,
               bool is_var_args);

  bool is_var_args_;
  std::uint32_t func_spec_{};
  QualType return_type_;
  std::vector<std::shared_ptr<Object>> params_;
};

}  // namespace kcc

#endif  // KCC_SRC_TYPE_H_
