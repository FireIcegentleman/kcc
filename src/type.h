//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_TYPE_H_
#define KCC_SRC_TYPE_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace kcc {

class PointerType;

class Type {
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

  virtual std::string Dump() const = 0;
  TypeId GetTypeId() const;
  bool IsVoidTy() const;
  bool IsFloatTy() const;
  bool IsDoubleTy() const;
  bool IsX86Fp80Ty() const;
  bool IsIntegerTy() const;
  bool IsIntegerTy(std::int32_t bit_width) const;
  bool IsFunctionTy() const;
  bool IsStructTy() const;
  bool IsArrayTy() const;
  bool IsPointerTy() const;
  std::int32_t GetIntegerBitWidth() const;
  std::shared_ptr<PointerType> GetPointerTo() const;
  std::shared_ptr<Type> GetArrayElementType() const;
  std::size_t GetArrayNumElements() const;
  std::shared_ptr<Type> GetStructElementType(std::int32_t i) const;
  std::int32_t GetStructNumElements() const;
  std::string GetStructName() const;
  bool IsFunctionVarArg() const;
  std::int32_t GetFunctionNumParams() const;
  std::shared_ptr<Type> GetFunctionParamType(std::int32_t i) const;
  bool IsUnsigned() const;
  virtual bool Compatible(std::shared_ptr<Type> other) const = 0;
  virtual std::int32_t Width() const = 0;
  virtual std::int32_t Align() const { return Width(); }
  bool Complete() const { return complete_; }
  void SetComplete(bool complete) const { complete_ = complete; }

  explicit Type(bool complete) : complete_{complete} {}
  ~Type() = default;

  // Type groups
  bool IsObjectTy() const { return !IsFunctionTy(); }
  bool IsArithmeticTy() const {
    return IsIntegerTy() || IsFloatTy() || IsDoubleTy() || IsX86Fp80Ty();
  }
  bool IsScalarTy() const { return IsArithmeticTy() || IsPointerTy(); }
  bool IsAggregateTy() const { return IsArrayTy() || IsStructTy(); }
  bool IsDerivedTy() const {
    return IsArrayTy() || IsFunctionTy() || IsStructTy();
  }
  bool IsConstQualified() const;
  bool IsRestrictQualified() const;
  bool IsVolatileQualified() const;
  static std::shared_ptr<Type> ArithmeticConversions(std::shared_ptr<Type> lhs,
                                                     std::shared_ptr<Type> rhs);
  bool Equal(std::shared_ptr<Type> other) const;

 private:
  // 不完整类型是缺乏足以确定其对象大小的信息对象类型。不完整类型可以在翻译单元的某些点完整。
  // 下列类型不完整：
  // 类型 void 。此类型不能完整。
  // 大小未知的数组。之后指定大小的声明能使之完整。
  // 内容未知的结构体或联合体类型。
  mutable bool complete_;

  std::uint32_t type_qualifiers_{kNone};
};

class VoidType : public Type {
 public:
  VoidType() : Type{false} {}

  virtual bool Compatible(const Type& other) const override {
    return other.IsVoidType();
  }
  virtual std::string Dump() const override { return "void"; }
  virtual std::int32_t Width() const override {
    // GNU 扩展
    return 1;
  }
  virtual bool IsVoidType() const override { return true; }

 private:
};

class PointerType : public Type {
 public:
  explicit PointerType(QualifierType derived) : Type{true}, derived_{derived} {}

  virtual bool Compatible(const Type& other) const override {
    return other.IsVoidType();
  }
  virtual std::string Dump() const override { return derived_.Dump() + "*"; }
  virtual std::int32_t Width() const override { return 8; }
  virtual bool IsPointerType() const override { return true; }

 private:
};

}  // namespace kcc

#endif  // KCC_SRC_TYPE_H_
