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

class Object;
class Scope;
class IntegerType;
class PointerType;

class Type : public std::enable_shared_from_this<Type> {
 public:
  enum TypeId {
    kVoidTyId,
    kFloatTyId,
    kDoubleTyId,
    kX86Fp80TyId,
    kIntegerTyId,
    kFunctionTyId,
    kStructTyId,
    kArrayTyId,
    kPointerTyId,
  };

  virtual ~Type() = default;

  virtual std::string ToString() const;
  virtual std::int32_t Width() const;
  void SetAlign(std::int32_t align);
  std::int32_t Align() const;
  bool HasAlign() const { return align_ != 0; }
  static std::shared_ptr<Type> MayCast(std::shared_ptr<Type> type,
                                       bool in_proto = false);

  virtual bool Compatible(const std::shared_ptr<Type>& other) const;
  virtual bool Equal(const std::shared_ptr<Type>& type) const;
  static std::shared_ptr<Type> IntegerPromote(std::shared_ptr<Type> type);
  static std::shared_ptr<Type> MaxType(std::shared_ptr<Type>& lhs,
                                       std::shared_ptr<Type>& rhs);
  std::int32_t Rank() const;
  bool IsTypedef() const { return storage_class_spec_ & kTypedef; }
  bool IsExtern() const { return storage_class_spec_ & kExtern; }
  void SetUnsigned() { type_spec_ |= kUnsigned; }

  bool IsComplete() const;
  void SetComplete(bool complete) const;
  bool IsVoidTy() const;
  bool IsBoolTy() const;
  bool IsFloatTy() const;
  bool IsDoubleTy() const;
  bool IsX86Fp80Ty() const;
  bool IsIntegerTy() const;
  bool IsIntegerTy(std::int32_t bit_width) const;
  bool IsFunctionTy() const;
  bool IsStructTy() const;
  bool IsArrayTy() const;
  bool IsPointerTy() const;
  bool IsObjectTy() const;
  bool IsArithmeticTy() const;
  bool IsScalarTy() const;
  bool IsRealTy() const;
  bool IsRealFloatPointTy() const;
  bool IsFloatPointTy() const;
  bool IsAggregateTy() const;
  bool IsDerivedTy() const;

  bool HasStructName() const;
  bool IsConstQualified() const;
  bool IsRestrictQualified() const;
  bool IsVolatileQualified() const;
  void SetConstQualified();
  void SetRestrictQualified();
  void SetVolatileQualified();
  bool IsUnsigned() const;
  bool IsStatic() const;
  std::int32_t GetAlign() const;
  void SetComplete(bool complete) { complete_ = complete; }

  void SetTypeQualifiers(std::uint32_t type_qualifiers);
  void SetStorageClassSpec(std::uint32_t storage_class_spec);

  std::shared_ptr<Type> GetFunctionParamType(std::int32_t i) const;
  std::int32_t GetFunctionNumParams() const;
  bool IsFunctionVarArg() const;
  std::vector<std::shared_ptr<Object>> GetFunctionParams() const;
  bool IsFunctionVarArgs() const;
  std::shared_ptr<Type> GetFunctionReturnType() const;

  std::string GetStructName() const;
  std::int32_t GetStructNumElements() const;
  std::shared_ptr<Type> GetStructElementType(std::int32_t i) const;
  std::shared_ptr<Object> GetStructMember(const std::string& name) const;

  std::size_t GetArrayNumElements() const;
  std::shared_ptr<Type> GetArrayElementType() const;

  std::shared_ptr<Type> GetPointerElementType() const;

  static std::shared_ptr<Type> GetVoidTy();
  static std::shared_ptr<Type> GetFloatTy();
  static std::shared_ptr<Type> GetDoubleTy();
  static std::shared_ptr<Type> GetX86Fp80Ty();

  static std::shared_ptr<Type> Get(std::uint32_t type_spec);

  std::shared_ptr<PointerType> GetPointerTo();
  static std::shared_ptr<PointerType> GetVoidPtrTy();

  void SetFuncSpec(std::uint32_t func_spec);
  bool IsInline() const;
  bool IsNoreturn() const;

 protected:
  explicit Type(TypeId type_id) : type_id_{type_id} {}
  std::int32_t num_bit_;

 private:
  TypeId type_id_;

  // 不完整类型是缺乏足以确定其对象大小的信息对象类型。不完整类型可以在翻译单元的某些点完整。
  // 下列类型不完整：
  // 类型 void 。此类型不能完整。
  // 大小未知的数组。之后指定大小的声明能使之完整。
  // 内容未知的结构体或联合体类型。
  mutable bool complete_;

  std::uint32_t type_spec_{};
  std::uint32_t type_qualifiers_{};
  std::uint32_t storage_class_spec_{};
  std::uint32_t func_spec_{};
  std::int32_t align_{};
};

class IntegerType : public Type {
  friend class Type;

 public:
  static std::shared_ptr<IntegerType> Get(std::int32_t num_bit);

  virtual std::string ToString() const override;
  virtual std::int32_t Width() const override { return num_bit_; }
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& type) const override;

 protected:
  explicit IntegerType(std::int32_t num_bit) : Type{kIntegerTyId} {
    num_bit_ = num_bit;
  }

 private:
};

class FunctionType : public Type {
 public:
  FunctionType(const FunctionType&) = delete;
  FunctionType& operator=(const FunctionType&) = delete;

  static std::shared_ptr<FunctionType> Get(
      std::shared_ptr<Type> return_type,
      std::vector<std::shared_ptr<Object>> params, bool is_var_args = false) {
    return std::shared_ptr<FunctionType>(
        new FunctionType{return_type, params, is_var_args});
  }

  std::shared_ptr<Type> GetParamType(std::int32_t i) const;
  std::int32_t GetNumParams() const { return std::size(params_); }
  bool IsVarArgs() const { return is_var_args_; }
  virtual std::string ToString() const override;
  std::shared_ptr<Type> GetReturnType() const;
  std::vector<std::shared_ptr<Object>> GetParams() const;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& type) const override;

 private:
  FunctionType(std::shared_ptr<Type> return_type,
               std::vector<std::shared_ptr<Object>> param, bool is_var_args)
      : Type{kFunctionTyId},
        return_type_{return_type},
        params_{param},
        is_var_args_{is_var_args} {}

  std::shared_ptr<Type> return_type_;
  std::vector<std::shared_ptr<Object>> params_;
  bool is_var_args_;
};

class StructType : public Type {
 public:
  static std::shared_ptr<StructType> Get(bool is_struct, bool has_name,
                                         std::shared_ptr<Scope> parent) {
    return std::shared_ptr<StructType>(
        new StructType{is_struct, has_name, parent});
  }

  virtual std::int32_t Width() const override { return width_; }
  void SetName(const std::string& name) { name_ = name; }
  std::string GetName() const;
  std::int32_t GetNumElements() const;
  std::shared_ptr<Type> GetElementType(std::int32_t i) const;

  void AddMember(std::shared_ptr<Object> member);
  void MergeAnony(std::shared_ptr<Object> anony);

  bool IsStruct() const { return is_struct_; }
  std::vector<std::shared_ptr<Object>> GetMembers() { return members_; }
  std::int32_t GetOffset() const { return offset_; }
  bool HasName() const { return has_name_; }
  std::shared_ptr<Scope> GetScope() { return scope_; }
  virtual std::string ToString() const override;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& type) const override;
  std::shared_ptr<Object> GetMember(const std::string& name) const;

  // 计算新成员的开始位置
  static int MakeAlign(int offset, int align) {
    if (offset % align == 0) {
      return offset;
    } else {
      return offset + align - offset % align;
    }
  }

 private:
  StructType(bool is_struct, bool has_name, std::shared_ptr<Scope> parent)
      : Type{kStructTyId},
        is_struct_{is_struct},
        has_name_{has_name},
        scope_{parent} {}

  std::string name_;

  std::vector<std::shared_ptr<Object>> members_;

  bool is_struct_;
  bool has_name_;
  std::shared_ptr<Scope> scope_;
  std::int32_t offset_;
  std::int32_t width_;
  std::int32_t align_;
};

class ArrayType : public Type {
 public:
  std::size_t GetNumElements() const { return num_elements_; }
  std::shared_ptr<Type> GetElementType() const { return contained_type_; }

  static std::shared_ptr<ArrayType> Get(std::shared_ptr<Type> contained_type,
                                        std::size_t num_elements);
  virtual std::string ToString() const override;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& type) const override;
  virtual std::int32_t Width() const override {
    return contained_type_->Width() * num_elements_;
  }

 private:
  ArrayType(std::shared_ptr<Type> contained_type, std::size_t num_elements)
      : Type{kArrayTyId},
        contained_type_{contained_type},
        num_elements_{num_elements} {
    SetComplete(num_elements_ > 0);
  }

  std::shared_ptr<Type> contained_type_;
  std::size_t num_elements_;
};

class PointerType : public Type {
 public:
  PointerType(const PointerType&) = delete;
  PointerType& operator=(const PointerType&) = delete;

  static std::shared_ptr<PointerType> Get(std::shared_ptr<Type> element_type) {
    return std::shared_ptr<PointerType>(new PointerType{element_type});
  }
  virtual std::string ToString() const override;
  std::shared_ptr<Type> GetElementType() const;
  virtual bool Compatible(const std::shared_ptr<Type>& other) const override;
  virtual bool Equal(const std::shared_ptr<Type>& type) const override;
  virtual std::int32_t Width() const override { return 8; }

 private:
  explicit PointerType(std::shared_ptr<Type> element_type)
      : Type{kPointerTyId}, element_type_{element_type} {}

  std::shared_ptr<Type> element_type_;
};

}  // namespace kcc

#endif  // KCC_SRC_TYPE_H_
