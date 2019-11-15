//
// Created by kaiser on 2019/10/31.
//

#include "type.h"

#include <cassert>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_os_ostream.h>

#include "ast.h"
#include "error.h"
#include "memory_pool.h"
#include "scope.h"
#include "util.h"

namespace kcc {

/*
 * QualType
 */
QualType::QualType(Type* type, std::uint32_t type_qual)
    : type_{type}, type_qual_{type_qual} {}

QualType::QualType(const QualType& item) {
  type_ = item.type_;
  type_qual_ = item.type_qual_;
}

QualType& QualType::operator=(const QualType& item) {
  type_ = item.type_;
  type_qual_ = item.type_qual_;
  return *this;
}

Type& QualType::operator*() { return *type_; }

const Type& QualType::operator*() const { return *type_; }

Type* QualType::operator->() { return type_; }

const Type* QualType::operator->() const { return type_; }

Type* QualType::GetType() {
  assert(type_ != nullptr);
  return type_;
}

const Type* QualType::GetType() const {
  assert(type_ != nullptr);
  return type_;
}

std::uint32_t QualType::GetTypeQual() const { return type_qual_; }

bool QualType::IsConst() const { return type_qual_ & kConst; }

bool QualType::IsRestrict() const { return type_qual_ & kRestrict; }

bool operator==(QualType lhs, QualType rhs) { return lhs.type_ == rhs.type_; }

bool operator!=(QualType lhs, QualType rhs) { return !(lhs == rhs); }

/*
 * Type
 */
QualType Type::MayCast(QualType type) {
  if (type->IsFunctionTy()) {
    return type->GetPointerTo();
  } else if (type->IsArrayTy()) {
    return PointerType::Get(type->ArrayGetElementType());
  } else {
    return type;
  }
}

std::string Type::ToString() const {
  std::string s;
  llvm::raw_string_ostream rso{s};
  llvm_type_->print(rso);

  if (IsLongTy()) {
    s = "l" + s;
  } else if (IsLongLongTy()) {
    s = "ll" + s;
  }

  if (IsIntegerTy() && IsUnsigned()) {
    s = "u" + s;
  }
  return rso.str();
}

llvm::Type* Type::GetLLVMType() const { return llvm_type_; }

bool Type::IsComplete() const { return complete_; }

void Type::SetComplete(bool complete) {
  complete_ = complete;

  if (IsStructOrUnionTy()) {
    StructFinish();
  } else if (IsArrayTy()) {
    assert(ArrayGetNumElements() != 0);
    ArrayFinish();
  }
}

bool Type::IsUnsigned() const {
  if (!IsIntegerTy()) {
    return false;
  } else {
    auto p{dynamic_cast<const ArithmeticType*>(this)};
    return p->type_spec_ & kUnsigned;
  }
}

bool Type::IsVoidTy() const { return dynamic_cast<const VoidType*>(this); }

bool Type::IsBoolTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kBool);
}

bool Type::IsShortTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kShort);
}

bool Type::IsIntTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kInt);
}

bool Type::IsLongTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kLong);
}

bool Type::IsLongLongTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kLongLong);
}

bool Type::IsFloatTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kFloat);
}

bool Type::IsDoubleTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kDouble);
}

bool Type::IsLongDoubleTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ == (kLong | kDouble));
}

bool Type::IsComplexTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kComplex);
}

bool Type::IsTypeName() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kTypedefName);
}

bool Type::IsPointerTy() const {
  return dynamic_cast<const PointerType*>(this);
}

bool Type::IsArrayTy() const { return dynamic_cast<const ArrayType*>(this); }

bool Type::IsStructTy() const {
  auto p{dynamic_cast<const StructType*>(this)};
  return p && p->is_struct_;
}

bool Type::IsUnionTy() const {
  auto p{dynamic_cast<const StructType*>(this)};
  return p && !p->IsStructTy();
}

bool Type::IsStructOrUnionTy() const { return IsStructTy() || IsUnionTy(); }

bool Type::IsFunctionTy() const {
  return dynamic_cast<const FunctionType*>(this);
}

bool Type::IsObjectTy() const { return !IsFunctionTy(); }

bool Type::IsCharacterTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kChar);
}

bool Type::IsIntegerTy() const {
  return dynamic_cast<const ArithmeticType*>(this) && !IsFloatPointTy() &&
         !IsBoolTy();
}

bool Type::IsRealTy() const { return IsIntegerTy() || IsRealFloatPointTy(); }

bool Type::IsArithmeticTy() const {
  return IsBoolTy() || IsIntegerTy() || IsFloatPointTy();
}

bool Type::IsScalarTy() const { return IsArithmeticTy() || IsPointerTy(); }

bool Type::IsAggregateTy() const { return IsArrayTy() || IsStructTy(); }

bool Type::IsDerivedTy() const {
  return IsArrayTy() || IsFunctionTy() || IsPointerTy();
}

bool Type::IsRealFloatPointTy() const {
  return IsFloatTy() || IsDoubleTy() || IsLongDoubleTy();
}

bool Type::IsFloatPointTy() const {
  // 不支持虚浮点数
  return IsRealFloatPointTy();
}

PointerType* Type::GetPointerTo() { return PointerType::Get(this); }

std::int32_t Type::ArithmeticRank() const {
  return dynamic_cast<const ArithmeticType*>(this)->Rank();
}

std::uint64_t Type::ArithmeticMaxIntegerValue() const {
  return dynamic_cast<const ArithmeticType*>(this)->MaxIntegerValue();
}

void Type::ArithmeticSetUnsigned() {
  dynamic_cast<ArithmeticType*>(this)->SetUnsigned();
}

QualType Type::PointerGetElementType() const {
  return dynamic_cast<const PointerType*>(this)->GetElementType();
}

void Type::ArraySetNumElements(std::size_t num_elements) {
  dynamic_cast<ArrayType*>(this)->SetNumElements(num_elements);
}

std::size_t Type::ArrayGetNumElements() const {
  return dynamic_cast<const ArrayType*>(this)->GetNumElements();
}

QualType Type::ArrayGetElementType() const {
  return dynamic_cast<const ArrayType*>(this)->GetElementType();
}

void Type::ArrayFinish() { dynamic_cast<ArrayType*>(this)->Finish(); }

bool Type::StructOrUnionHasName() const {
  return dynamic_cast<const StructType*>(this)->HasName();
}

void Type::StructSetName(const std::string& name) {
  dynamic_cast<StructType*>(this)->SetName(name);
}

std::string Type::StructGetName() const {
  return dynamic_cast<const StructType*>(this)->GetName();
}

std::int32_t Type::StructGetNumMembers() const {
  return dynamic_cast<const StructType*>(this)->GetNumMembers();
}

std::vector<ObjectExpr*>& Type::StructGetMembers() {
  return dynamic_cast<StructType*>(this)->GetMembers();
}

ObjectExpr* Type::StructGetMember(const std::string& name) const {
  return dynamic_cast<const StructType*>(this)->GetMember(name);
}

QualType Type::StructGetMemberType(std::int32_t i) const {
  return dynamic_cast<const StructType*>(this)->GetMemberType(i);
}

Scope* Type::StructGetScope() {
  return dynamic_cast<StructType*>(this)->GetScope();
}

void Type::StructAddMember(ObjectExpr* member) {
  dynamic_cast<StructType*>(this)->AddMember(member);
}

void Type::StructMergeAnonymous(ObjectExpr* anonymous) {
  dynamic_cast<StructType*>(this)->MergeAnonymous(anonymous);
}

std::int32_t Type::StructGetOffset() const {
  return dynamic_cast<const StructType*>(this)->GetOffset();
}

void Type::StructFinish() { dynamic_cast<StructType*>(this)->Finish(); }

bool Type::StructHasFlexibleArray() const {
  assert(IsStructTy());
  return dynamic_cast<const StructType*>(this)->HasFlexibleArray();
}

bool Type::FuncIsVarArgs() const {
  return dynamic_cast<const FunctionType*>(this)->IsVarArgs();
}

QualType Type::FuncGetReturnType() const {
  return dynamic_cast<const FunctionType*>(this)->GetReturnType();
}

std::int32_t Type::FuncGetNumParams() const {
  return dynamic_cast<const FunctionType*>(this)->GetNumParams();
}

QualType Type::FuncGetParamType(std::int32_t i) const {
  return dynamic_cast<const FunctionType*>(this)->GetParamType(i);
}

std::vector<ObjectExpr*>& Type::FuncGetParams() {
  return dynamic_cast<FunctionType*>(this)->GetParams();
}

void Type::FuncSetFuncSpec(std::uint32_t func_spec) {
  return dynamic_cast<FunctionType*>(this)->SetFuncSpec(func_spec);
}

bool Type::FuncIsInline() const {
  return dynamic_cast<const FunctionType*>(this)->IsInline();
}

bool Type::FuncIsNoreturn() const {
  return dynamic_cast<const FunctionType*>(this)->IsNoreturn();
}

void Type::FuncSetName(const std::string& name) {
  dynamic_cast<FunctionType*>(this)->SetName(name);
}

std::string Type::FuncGetName() const {
  return dynamic_cast<const FunctionType*>(this)->GetName();
}

Type::Type(bool complete) : complete_{complete} {}

/*
 * VoidType
 */
VoidType* VoidType::Get() {
  static auto type{new (VoidTypePool.Allocate()) VoidType{}};
  return type;
}

std::int32_t VoidType::GetWidth() const {
  // GNU 扩展
  return 1;
}

std::int32_t VoidType::GetAlign() const { return GetWidth(); }

bool VoidType::Compatible(const Type* other) const { return Equal(other); }

bool VoidType::Equal(const Type* other) const { return other->IsVoidTy(); }

VoidType::VoidType() : Type{false} { llvm_type_ = Builder.getVoidTy(); }

/*
 * ArithmeticType
 */
ArithmeticType* ArithmeticType::Get(std::uint32_t type_spec) {
  type_spec = ArithmeticType::DealWithTypeSpec(type_spec);

  switch (type_spec) {
    case kBool:
      return new (ArithmeticTypePool.Allocate()) ArithmeticType{kBool};
    case kChar:
      return new (ArithmeticTypePool.Allocate()) ArithmeticType{kChar};
    case kChar | kUnsigned:
      return new (ArithmeticTypePool.Allocate())
          ArithmeticType{kChar | kUnsigned};
    case kShort:
      return new (ArithmeticTypePool.Allocate()) ArithmeticType{kShort};
    case kShort | kUnsigned:
      return new (ArithmeticTypePool.Allocate())
          ArithmeticType{kShort | kUnsigned};
    case kInt:
      return new (ArithmeticTypePool.Allocate()) ArithmeticType{kInt};
    case kInt | kUnsigned:
      return new (ArithmeticTypePool.Allocate())
          ArithmeticType{kInt | kUnsigned};
    case kLong:
      return new (ArithmeticTypePool.Allocate()) ArithmeticType{kLong};
    case kLong | kUnsigned:
      return new (ArithmeticTypePool.Allocate())
          ArithmeticType{kLong | kUnsigned};
    case kLongLong:
      return new (ArithmeticTypePool.Allocate()) ArithmeticType{kLongLong};
    case kLongLong | kUnsigned:
      return new (ArithmeticTypePool.Allocate())
          ArithmeticType{kLongLong | kUnsigned};
    case kFloat:
      return new (ArithmeticTypePool.Allocate()) ArithmeticType{kFloat};
    case kDouble:
      return new (ArithmeticTypePool.Allocate()) ArithmeticType{kDouble};
    case kDouble | kLong:
      return new (ArithmeticTypePool.Allocate())
          ArithmeticType{kDouble | kLong};
    default:
      assert(false);
      return nullptr;
  }
}

Type* ArithmeticType::IntegerPromote(Type* type) {
  assert(type->IsIntegerTy() || type->IsBoolTy());

  static auto int_type{ArithmeticType::Get(kInt)};
  if (type->ArithmeticRank() < int_type->Rank()) {
    return int_type;
  } else {
    return type;
  }
}

Type* ArithmeticType::MaxType(Type* lhs, Type* rhs) {
  assert(lhs->IsArithmeticTy() && rhs->IsArithmeticTy());

  if ((!lhs->IsIntegerTy() || !rhs->IsIntegerTy()) &&
      (!lhs->IsBoolTy() && !rhs->IsBoolTy())) {
    return lhs->ArithmeticRank() >= rhs->ArithmeticRank() ? lhs : rhs;
  } else {
    lhs = ArithmeticType::IntegerPromote(lhs);
    rhs = ArithmeticType::IntegerPromote(rhs);

    if (lhs->Equal(rhs)) {
      return lhs;
    } else if (lhs->IsUnsigned() == rhs->IsUnsigned()) {
      return lhs->ArithmeticRank() >= rhs->ArithmeticRank() ? lhs : rhs;
    } else {
      auto unsigned_type{lhs->IsUnsigned() ? lhs : rhs};
      auto signed_type{lhs->IsUnsigned() ? rhs : lhs};

      if (unsigned_type->ArithmeticRank() >= signed_type->ArithmeticRank()) {
        return unsigned_type;
      } else {
        if (signed_type->ArithmeticMaxIntegerValue() >=
            unsigned_type->ArithmeticMaxIntegerValue()) {
          return signed_type;
        } else {
          signed_type->ArithmeticSetUnsigned();
          return signed_type;
        }
      }
    }
  }
}

std::int32_t ArithmeticType::GetWidth() const {
  switch (type_spec_) {
    case kBool:
      return 1;
    case kChar:
    case kChar | kUnsigned:
      return 1;
    case kShort:
    case kShort | kUnsigned:
      return 2;
    case kInt:
    case kInt | kUnsigned:
      return 4;
    case kLong:
    case kLong | kUnsigned:
    case kLongLong:
    case kLongLong | kUnsigned:
      return 8;
    case kFloat:
      return 4;
    case kDouble:
      return 8;
    case kDouble | kLong:
      return 16;
    default:
      assert(false);
      return 0;
  }
}

std::int32_t ArithmeticType::GetAlign() const { return GetWidth(); }

// 它们是同一类型(同名或由 typedef 引入的别名)
// 类型 char 不与 signed char 兼容, 且不与 unsigned char 兼容
bool ArithmeticType::Compatible(const Type* other) const {
  return Equal(other);
}

bool ArithmeticType::Equal(const Type* type) const {
  if (type->IsArithmeticTy()) {
    return type_spec_ == dynamic_cast<const ArithmeticType*>(type)->type_spec_;
  } else {
    return false;
  }
}

std::uint64_t ArithmeticType::MaxIntegerValue() const {
  switch (type_spec_) {
    case kChar:
      return std::numeric_limits<std::int8_t>::max();
    case kChar | kUnsigned:
      return std::numeric_limits<std::uint8_t>::max();
    case kShort:
      return std::numeric_limits<std::int16_t>::max();
    case kShort | kUnsigned:
      return std::numeric_limits<std::uint16_t>::max();
    case kInt:
      return std::numeric_limits<std::int32_t>::max();
    case kInt | kUnsigned:
      return std::numeric_limits<std::uint32_t>::max();
    case kLong:
    case kLongLong:
      return std::numeric_limits<std::int64_t>::max();
    case kLong | kUnsigned:
    case kLongLong | kUnsigned:
      return std::numeric_limits<std::uint64_t>::max();
    default:
      assert(false);
      return 0;
  }
}

void ArithmeticType::SetUnsigned() {
  assert(!IsBoolTy() && !IsFloatPointTy());
  type_spec_ |= kUnsigned;
}

ArithmeticType::ArithmeticType(std::uint32_t type_spec) : Type{true} {
  type_spec_ = ArithmeticType::DealWithTypeSpec(type_spec);

  if (IsBoolTy()) {
    llvm_type_ = Builder.getInt1Ty();
  } else if (IsCharacterTy()) {
    llvm_type_ = Builder.getInt8Ty();
  } else if (IsShortTy()) {
    llvm_type_ = Builder.getInt16Ty();
  } else if (IsIntTy()) {
    llvm_type_ = Builder.getInt32Ty();
  } else if (IsLongTy() || IsLongLongTy()) {
    llvm_type_ = Builder.getInt64Ty();
  } else if (IsFloatTy()) {
    llvm_type_ = Builder.getFloatTy();
  } else if (IsDoubleTy()) {
    llvm_type_ = Builder.getDoubleTy();
  } else if (IsLongDoubleTy()) {
    llvm_type_ = llvm::Type::getX86_FP80Ty(Context);
  } else {
    assert(false);
  }
}

std::uint32_t ArithmeticType::DealWithTypeSpec(std::uint32_t type_spec) {
  if (type_spec == kSigned) {
    type_spec = kInt;
  } else if (type_spec == kUnsigned) {
    type_spec |= kInt;
  }

  type_spec &= ~kSigned;

  if ((type_spec & kShort) || (type_spec & kLong) || (type_spec & kLongLong)) {
    type_spec &= ~kInt;
  }

  return type_spec;
}

std::int32_t ArithmeticType::Rank() const {
  switch (type_spec_) {
    case kBool:
      return 0;
    case kChar:
    case kChar | kUnsigned:
      return 1;
    case kShort:
    case kShort | kUnsigned:
      return 2;
    case kInt:
    case kInt | kUnsigned:
      return 3;
    case kLong:
    case kLong | kUnsigned:
      return 4;
    case kLongLong:
    case kLongLong | kUnsigned:
      return 5;
    case kFloat:
      return 6;
    case kDouble:
      return 7;
    case kDouble | kLong:
      return 8;
    default:
      assert(false);
      return 0;
  }
}

/*
 * PointerType
 */
PointerType* PointerType::Get(QualType element_type) {
  return new (PointerTypePool.Allocate()) PointerType{element_type};
}

std::int32_t PointerType::GetWidth() const { return 8; }

std::int32_t PointerType::GetAlign() const { return GetWidth(); }

// 指向兼容类型
bool PointerType::Compatible(const Type* other) const {
  if (other->IsPointerTy()) {
    return element_type_->Compatible(
        dynamic_cast<const PointerType*>(other)->element_type_.GetType());
  } else {
    return false;
  }
}

bool PointerType::Equal(const Type* other) const {
  if (other->IsPointerTy()) {
    return element_type_->Equal(
        dynamic_cast<const PointerType*>(other)->element_type_.GetType());
  } else {
    return false;
  }
}

QualType PointerType::GetElementType() const { return element_type_; }

PointerType::PointerType(QualType element_type)
    : Type{true}, element_type_{element_type} {
  if (element_type_->IsVoidTy()) {
    llvm_type_ = Builder.getInt8PtrTy();
  } else {
    llvm_type_ = element_type_->GetLLVMType()->getPointerTo();
  }
}

/*
 * ArrayType
 */
ArrayType* ArrayType::Get(QualType contained_type, std::size_t num_elements) {
  return new (ArrayTypePool.Allocate()) ArrayType{contained_type, num_elements};
}

std::int32_t ArrayType::GetWidth() const {
  return contained_type_->GetWidth() * num_elements_;
}

std::int32_t ArrayType::GetAlign() const { return contained_type_->GetAlign(); }

// 其元素类型兼容, 且
// 若都拥有常量大小, 则大小相同
// 注意: 未知边界数组与任何兼容元素类型的数组兼容
// VLA 与任何兼容元素类型的数组兼容
bool ArrayType::Compatible(const Type* other) const {
  if (other->IsArrayTy()) {
    auto other_arr{dynamic_cast<const ArrayType*>(other)};
    if (!contained_type_->Compatible(other_arr->contained_type_.GetType())) {
      return false;
    }
    if (IsComplete() && other_arr->IsComplete()) {
      return GetNumElements() == other_arr->GetNumElements();
    }

    return true;
  } else {
    return false;
  }
}

bool ArrayType::Equal(const Type* other) const {
  if (other->IsArrayTy()) {
    auto other_arr{dynamic_cast<const ArrayType*>(other)};
    if (!contained_type_->Equal(other_arr->contained_type_.GetType())) {
      return false;
    }
    if (IsComplete() && other_arr->IsComplete()) {
      return GetNumElements() == other_arr->GetNumElements();
    }

    return true;
  } else {
    return false;
  }
}

void ArrayType::SetNumElements(std::size_t num_elements) {
  num_elements_ = num_elements;
}

std::size_t ArrayType::GetNumElements() const { return num_elements_; }

QualType ArrayType::GetElementType() const { return contained_type_; }

void ArrayType::Finish() {
  llvm_type_ =
      llvm::ArrayType::get(contained_type_->GetLLVMType(), num_elements_);
}

ArrayType::ArrayType(QualType contained_type, std::size_t num_elements)
    : Type{num_elements > 0},
      contained_type_{contained_type},
      num_elements_{num_elements} {
  llvm_type_ =
      llvm::ArrayType::get(contained_type_->GetLLVMType(), num_elements_);
}

/*
 * StructType
 */
StructType* StructType::Get(bool is_struct, const std::string& name,
                            Scope* parent) {
  return new (StructTypePool.Allocate()) StructType{is_struct, name, parent};
}

std::int32_t StructType::GetWidth() const {
  if (std::size(members_) == 0) {
    return 1;
  } else {
    return width_;
  }
}

std::int32_t StructType::GetAlign() const {
  if (std::size(members_) == 0) {
    return 1;
  } else {
    return align_;
  }
}

// 若一者以标签声明, 则另一者必须以同一标签声明。
// 若它们都是完整类型, 则其成员必须在数量上准确对应, 以兼容类型声明,
// 并拥有匹配的名称 另外, 若它们都是枚举, 则对应成员亦必须拥有相同值。 另外,
// 若它们是结构体或联合体, 则 对应的元素必须以同一顺序声明(仅结构体)
// 对应的位域必须有相同宽度
bool StructType::Compatible(const Type* other) const {
  if (other->IsStructOrUnionTy() && IsStruct() == other->IsStructTy()) {
    auto other_struct{dynamic_cast<const StructType*>(other)};
    if (HasName() && other_struct->HasName()) {
      if (name_ != other_struct->name_) {
        return false;
      }
    }

    if (IsComplete() && other_struct->IsComplete()) {
      if (GetNumMembers() != other_struct->GetNumMembers()) {
        return false;
      }

      return GetLLVMType() == other_struct->GetLLVMType();
    }

    return true;
  } else {
    return false;
  }
}

bool StructType::Equal(const Type* other) const {
  if (other->IsStructOrUnionTy() && IsStruct() == other->IsStructTy()) {
    auto other_struct{dynamic_cast<const StructType*>(other)};
    if (HasName() && other_struct->HasName()) {
      if (name_ != other_struct->name_) {
        return false;
      }
    }

    if (IsComplete() && other_struct->IsComplete()) {
      if (GetNumMembers() != other_struct->GetNumMembers()) {
        return false;
      }

      return GetLLVMType() == other_struct->GetLLVMType();
    }

    return true;
  } else {
    return false;
  }
}

bool StructType::IsStruct() const { return is_struct_; }

bool StructType::HasName() const { return !std::empty(name_); }

void StructType::SetName(const std::string& name) {
  name_ = name;
  std::string pre{is_struct_ ? "struct." : "union."};
  llvm::cast<llvm::StructType>(llvm_type_)->setName(pre + name);
}

std::string StructType::GetName() const { return name_; }

std::int32_t StructType::GetNumMembers() const { return std::size(members_); }

std::vector<ObjectExpr*>& StructType::GetMembers() { return members_; }

ObjectExpr* StructType::GetMember(const std::string& name) const {
  auto iter{scope_->FindNormalInCurrScope(name)};
  if (iter == nullptr) {
    return nullptr;
  } else {
    return dynamic_cast<ObjectExpr*>(iter);
  }
}

QualType StructType::GetMemberType(std::int32_t i) const {
  return members_[i]->GetQualType();
}

Scope* StructType::GetScope() { return scope_; }

std::int32_t StructType::GetOffset() const { return offset_; }

void StructType::AddMember(ObjectExpr* member) {
  if (member->GetType()->IsArrayTy() && !member->GetType()->IsComplete()) {
    has_flexible_array_ = true;
  }

  auto offset{MakeAlign(offset_, member->GetAlign())};
  member->SetOffset(offset);

  members_.push_back(member);

  scope_->InsertNormal(member->GetName(), member);
  member->SetIndex(index_++);

  align_ = std::max(align_, member->GetAlign());

  if (is_struct_) {
    offset_ = offset + member->GetQualType()->GetWidth();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, member->GetQualType()->GetWidth());
    width_ = MakeAlign(width_, align_);
  }
}

void StructType::MergeAnonymous(ObjectExpr* anonymous) {
  // 匿名 struct / union
  auto annoy_type{dynamic_cast<const StructType*>(anonymous->GetType())};
  assert(annoy_type != nullptr);

  auto offset{MakeAlign(offset_, anonymous->GetAlign())};
  anonymous->SetOffset(offset);

  members_.push_back(anonymous);

  for (auto& [name, obj] : *annoy_type->scope_) {
    if (auto member{dynamic_cast<ObjectExpr*>(obj)}; !member) {
      continue;
    } else {
      if (GetMember(name)) {
        Error(member->GetLoc(), "duplicated member: {}", name);
      }

      member->SetOffset(offset + member->GetOffset());

      scope_->InsertNormal(name, member);
      member->SetIndex(index_++);
    }
  }

  align_ = std::max(align_, anonymous->GetAlign());

  if (is_struct_) {
    offset_ = offset + annoy_type->GetWidth();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, annoy_type->GetWidth());
    width_ = MakeAlign(width_, align_);
  }
}

void StructType::Finish() {
  std::vector<llvm::Type*> members;
  for (const auto& item : members_) {
    members.push_back(item->GetType()->GetLLVMType());
  }

  llvm::cast<llvm::StructType>(llvm_type_)->setBody(members);
}

bool StructType::HasFlexibleArray() const { return has_flexible_array_; }

StructType::StructType(bool is_struct, const std::string& name, Scope* parent)
    : Type{false},
      is_struct_{is_struct},
      name_{name},
      scope_{Scope::Get(parent, kBlock)} {
  std::string pre{is_struct ? "struct." : "union."};
  if (HasName()) {
    for (const auto& item : Module->getIdentifiedStructTypes()) {
      if (item->getName() == pre + name) {
        llvm_type_ = item;
        return;
      }
    }

    llvm_type_ = llvm::StructType::create(Context, pre + name);
  } else {
    llvm_type_ = llvm::StructType::create(Context, pre + "anon");
  }
}

std::int32_t StructType::MakeAlign(std::int32_t offset, std::int32_t align) {
  assert(align != 0);

  if (offset % align == 0) {
    return offset;
  } else {
    return offset + align - (offset % align);
  }
}

/*
 * FunctionType
 */
FunctionType* FunctionType::Get(QualType return_type,
                                std::vector<ObjectExpr*> params,
                                bool is_var_args) {
  return new (FunctionTypePool.Allocate())
      FunctionType{return_type, params, is_var_args};
}

std::int32_t FunctionType::GetWidth() const {
  // GNU 扩展
  return 1;
}

std::int32_t FunctionType::GetAlign() const {
  assert(false);
  return 0;
}

// 其返回类型兼容
// 它们都使用参数列表, 参数数量(包括省略号的使用)相同, 而其对应参数,
// 在应用数组到指针和函数到指针类型调整, 及剥除顶层限定符后, 拥有相同类型
bool FunctionType::Compatible(const Type* other) const {
  if (other->IsFunctionTy()) {
    auto other_func{dynamic_cast<const FunctionType*>(other)};
    if (!return_type_->Compatible(other_func->return_type_.GetType())) {
      return false;
    }
    if (GetNumParams() != other_func->GetNumParams()) {
      return false;
    }

    auto iter{std::begin(other_func->params_)};
    for (const auto& param : params_) {
      if (!param->GetType()->Equal((*iter)->GetType())) {
        return false;
      }
      ++iter;
    }

    return true;
  } else {
    return false;
  }
}

bool FunctionType::Equal(const Type* other) const {
  if (other->IsFunctionTy()) {
    auto other_func{dynamic_cast<const FunctionType*>(other)};
    if (!return_type_->Equal(other_func->return_type_.GetType())) {
      return false;
    }
    if (GetNumParams() != other_func->GetNumParams()) {
      return false;
    }

    auto iter{std::begin(other_func->params_)};
    for (const auto& param : params_) {
      if (!param->GetType()->Equal((*iter)->GetType())) {
        return false;
      }
      ++iter;
    }

    return true;
  } else {
    return false;
  }
}

bool FunctionType::IsVarArgs() const { return is_var_args_; }

QualType FunctionType::GetReturnType() const { return return_type_; }

std::int32_t FunctionType::GetNumParams() const { return std::size(params_); }

QualType FunctionType::GetParamType(std::int32_t i) const {
  return params_[i]->GetQualType();
}

std::vector<ObjectExpr*>& FunctionType::GetParams() { return params_; }

void FunctionType::SetFuncSpec(std::uint32_t func_spec) {
  func_spec_ = func_spec;
}

bool FunctionType::IsInline() const { return func_spec_ & kInline; }

bool FunctionType::IsNoreturn() const { return func_spec_ & kNoreturn; }

void FunctionType::SetName(const std::string& name) { name_ = name; }

std::string FunctionType::GetName() const { return name_; }

FunctionType::FunctionType(QualType return_type, std::vector<ObjectExpr*> param,
                           bool is_var_args)
    : Type{false},
      is_var_args_{is_var_args},
      return_type_{return_type},
      params_{param} {
  std::vector<llvm::Type*> params;
  for (const auto& item : params_) {
    params.push_back(item->GetType()->GetLLVMType());
  }

  llvm_type_ = llvm::FunctionType::get(return_type_->GetLLVMType(), params,
                                       is_var_args_);
}

}  // namespace kcc
