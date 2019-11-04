//
// Created by kaiser on 2019/10/31.
//

#include "type.h"

#include <cassert>

#include "ast.h"
#include "scope.h"

namespace kcc {

std::string Type::ToString() const {
  switch (type_id_) {
    case kVoidTyId:
      return "void";
    case kFloatTyId:
      return "float";
    case kDoubleTyId:
      return "double";
    case kX86Fp80TyId:
      return "long double";
    default:
      assert(false);
      return nullptr;
  }
}

std::int32_t Type::Width() const {
  switch (type_id_) {
    case kVoidTyId:
      return 1;
    case kFloatTyId:
      return 4;
    case kDoubleTyId:
      return 8;
    case kX86Fp80TyId:
      return 16;
    default:
      assert(false);
      return 0;
  }
}

std::int32_t Type::Align() const {
  assert(type_id_ != kFunctionTyId);

  if (align_) {
    return align_;
  } else {
    return Width();
  }
}

std::shared_ptr<Type> Type::MayCast(std::shared_ptr<Type> type, bool in_proto) {
  if (type->IsFunctionTy()) {
    return type->GetPointerTo();
  } else if (type->IsArrayTy()) {
    type = type->GetPointerTo();

    if (!in_proto) {
      type->SetConstQualified();
    }

    return type;
  } else {
    return type;
  }
}

bool Type::IsComplete() const {
  if (IsArrayTy() || IsStructTy()) {
    return complete_;
  } else {
    return true;
  }
}

void Type::SetComplete(bool complete) const {
  assert(type_id_ == kArrayTyId || type_id_ == kStructTyId);
  complete_ = complete;
}

bool Type::IsVoidTy() const { return type_id_ == kVoidTyId; }

bool Type::IsFloatTy() const { return type_id_ == kFloatTyId; }

bool Type::IsDoubleTy() const { return type_id_ == kDoubleTyId; }

bool Type::IsX86Fp80Ty() const { return type_id_ == kX86Fp80TyId; }

bool Type::IsIntegerTy() const { return type_id_ == kIntegerTyId; }

bool Type::IsIntegerTy(std::int32_t bit_width) const {
  return type_id_ == kIntegerTyId && Width() == bit_width;
}

bool Type::IsFunctionTy() const { return type_id_ == kFunctionTyId; }

bool Type::IsStructTy() const { return type_id_ == kStructTyId; }

bool Type::IsArrayTy() const { return type_id_ == kArrayTyId; }

bool Type::IsPointerTy() const { return type_id_ == kPointerTyId; }

bool Type::IsObjectTy() const { return !IsFunctionTy(); }

bool Type::IsArithmeticTy() const {
  return IsIntegerTy() || IsFloatTy() || IsDoubleTy() || IsX86Fp80Ty();
}

bool Type::IsScalarTy() const { return IsArithmeticTy() || IsPointerTy(); }

bool Type::IsAggregateTy() const { return IsArrayTy() || IsStructTy(); }

bool Type::IsDerivedTy() const {
  return IsArrayTy() || IsFunctionTy() || IsStructTy();
}

bool Type::IsConstQualified() const { return type_qualifiers_ & kConst; }

bool Type::IsRestrictQualified() const { return type_qualifiers_ & kRestrict; }

bool Type::IsVolatileQualified() const { return type_qualifiers_ & kVolatile; }

bool Type::IsUnsigned() const { return type_spec_ & kUnsigned; }

std::shared_ptr<Type> Type::GetFunctionParamType(std::int32_t i) const {
  return dynamic_cast<const FunctionType*>(this)->GetParamType(i);
}

std::int32_t Type::GetFunctionNumParams() const {
  return dynamic_cast<const FunctionType*>(this)->GetNumParams();
}

bool Type::IsFunctionVarArg() const {
  return dynamic_cast<const FunctionType*>(this)->IsVarArgs();
}

std::string Type::GetStructName() const {
  return dynamic_cast<const StructType*>(this)->GetName();
}

std::int32_t Type::GetStructNumElements() const {
  return dynamic_cast<const StructType*>(this)->GetNumElements();
}

std::shared_ptr<Type> Type::GetStructElementType(std::int32_t i) const {
  return dynamic_cast<const StructType*>(this)->GetElementType(i);
}

std::size_t Type::GetArrayNumElements() const {
  return dynamic_cast<const ArrayType*>(this)->GetNumElements();
}

std::shared_ptr<Type> Type::GetArrayElementType() const {
  return dynamic_cast<const ArrayType*>(this)->GetElementType();
}

std::shared_ptr<Type> Type::GetPointerElementType() const {
  return dynamic_cast<const PointerType*>(this)->GetElementType();
}

std::shared_ptr<Type> Type::GetVoidTy() {
  return std::shared_ptr<Type>(new Type{kVoidTyId});
}

std::shared_ptr<Type> Type::GetFloatTy() {
  return std::shared_ptr<Type>(new Type{kFloatTyId});
}

std::shared_ptr<Type> Type::GetDoubleTy() {
  return std::shared_ptr<Type>(new Type{kDoubleTyId});
}

std::shared_ptr<Type> Type::GetX86Fp80Ty() {
  return std::shared_ptr<Type>(new Type{kX86Fp80TyId});
}

std::shared_ptr<IntegerType> IntegerType::Get(std::int32_t num_bit) {
  return std::shared_ptr<IntegerType>(new IntegerType{num_bit});
}

std::shared_ptr<PointerType> Type::GetPointerTo() {
  return PointerType::Get(shared_from_this());
}

void Type::SetAlign(std::int32_t align) { align_ = align; }

void Type::SetTypeQualifiers(std::uint32_t type_qualifiers) {
  type_qualifiers_ = type_qualifiers;
}

void Type::SetStorageClassSpec(std::uint32_t storage_class_spec) {
  storage_class_spec_ = storage_class_spec;
}

std::shared_ptr<PointerType> Type::GetVoidPtrTy() {
  return PointerType::Get(Type::GetVoidTy());
}

std::shared_ptr<Type> Type::Get(std::uint32_t type_spec) {
  if (type_spec & kVoid) {
    auto ret{Type::GetVoidTy()};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & kFloat) {
    auto ret{Type::GetFloatTy()};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & kDouble) {
    auto ret{Type::GetDoubleTy()};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & (kLong | kDouble)) {
    auto ret{Type::GetX86Fp80Ty()};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & kChar) {
    auto ret{IntegerType::Get(8)};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & kShort) {
    auto ret{IntegerType::Get(16)};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & kInt) {
    auto ret{IntegerType::Get(32)};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & kLong) {
    auto ret{IntegerType::Get(64)};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & kLongLong) {
    auto ret{IntegerType::Get(64)};
    ret->type_spec_ = type_spec;
    return ret;
  } else if (type_spec & kBool) {
    auto ret{IntegerType::Get(1)};
    ret->type_spec_ = type_spec;
    return ret;
  } else {
    assert(false);
    return nullptr;
  }
}

bool Type::Compatible(const std::shared_ptr<Type>&) const { return true; }

bool Type::Equal(const std::shared_ptr<Type>& type) const {
  return type_spec_ == type->type_spec_ &&
         type_qualifiers_ == type->type_qualifiers_;
}

bool IntegerType::Compatible(const std::shared_ptr<Type>& other) const {
  return Type::Compatible(other);
}

bool IntegerType::Equal(const std::shared_ptr<Type>& type) const {
  return Type::Equal(type) && Width() == type->Width();
}

std::shared_ptr<Type> Type::IntegerPromote(std::shared_ptr<Type> type) {
  assert(type->IsIntegerTy());

  if (type->Rank() < IntegerType::Get(32)->Rank()) {
    type->num_bit_ = 32;
    type->type_spec_ = kInt;
  }

  return type;
}

std::shared_ptr<Type> Type::MaxType(std::shared_ptr<Type>& lhs,
                                    std::shared_ptr<Type>& rhs) {
  if (lhs->IsIntegerTy()) {
    lhs = Type::IntegerPromote(lhs);
  }
  if (rhs->IsIntegerTy()) {
    rhs = Type::IntegerPromote(rhs);
  }

  auto ret = lhs->Rank() > rhs->Rank() ? lhs : rhs;

  if (lhs->Width() == rhs->Width() &&
      (lhs->IsUnsigned() || rhs->IsUnsigned())) {
    return IntegerType::Get(ret->num_bit_);
  } else {
    return ret;
  }
}

bool Type::IsBoolTy() const { return IsIntegerTy(1); }

bool Type::IsRealTy() const { return IsIntegerTy() && IsRealFloatPointTy(); }

bool Type::IsRealFloatPointTy() const {
  return IsFloatTy() && IsDoubleTy() && IsX86Fp80Ty();
}

bool Type::IsFloatPointTy() const { return false; }

bool Type::IsStatic() const { return storage_class_spec_ & kStatic; }

std::int32_t Type::GetAlign() const {
  if (align_) {
    return align_;
  } else {
    return Width();
  }
}

std::vector<std::shared_ptr<Object>> Type::GetFunctionParams() const {
  return std::vector<std::shared_ptr<Object>>();
}

bool Type::IsFunctionVarArgs() const { return false; }
std::shared_ptr<Type> Type::GetFunctionReturnType() const {
  return std::shared_ptr<Type>();
}

void Type::SetFuncSpec(std::uint32_t func_spec) { func_spec_ = func_spec; }

bool Type::IsInline() const { return func_spec_ & kInline; }

bool Type::IsNoreturn() const { return func_spec_ & kNoreturn; }

std::int32_t Type::Rank() const {
  if (type_spec_ & (kLong | kDouble)) {
    return 8;
  } else if (type_spec_ & kDouble) {
    return 7;
  } else if (type_spec_ & kFloat) {
    return 6;
  } else if (type_spec_ & kLongLong) {
    return 5;
  } else if (type_spec_ & kLong) {
    return 4;
  } else if (type_spec_ & kInt) {
    return 3;
  } else if (type_spec_ & kShort) {
    return 2;
  } else if (type_spec_ & kChar) {
    return 1;
  } else if (type_spec_ & kBool) {
    return 0;
  } else {
    assert(false);
    return 0;
  }
}

bool Type::HasStructName() const {
  return dynamic_cast<const StructType*>(this)->HasName();
}

void Type::SetConstQualified() { type_qualifiers_ |= kConst; }

void Type::SetRestrictQualified() { type_qualifiers_ |= kRestrict; }

void Type::SetVolatileQualified() { type_qualifiers_ |= kVolatile; }

std::shared_ptr<Object> Type::GetStructMember(const std::string& name) const {
  return dynamic_cast<const StructType*>(this)->GetMember(name);
}

std::string IntegerType::ToString() const {
  return (IsUnsigned() ? "u" : "") + std::string("int") +
         std::to_string(Width());
}

std::shared_ptr<Type> FunctionType::GetParamType(std::int32_t i) const {
  return params_[i]->GetType();
}

std::string FunctionType::ToString() const {
  std::string s(return_type_->ToString() + "(");

  for (const auto& param : params_) {
    s += param->GetType()->ToString() + ", ";
  }

  if (is_var_args_) {
    s += "...)";
    return s;
  } else {
    s.pop_back();
    s.pop_back();

    return s + ")";
  }
}

std::shared_ptr<Type> FunctionType::GetReturnType() const {
  return return_type_;
}

std::vector<std::shared_ptr<Object>> FunctionType::GetParams() const {
  return params_;
}

bool FunctionType::Compatible(const std::shared_ptr<Type>& other) const {
  return Type::Compatible(other);
}

bool FunctionType::Equal(const std::shared_ptr<Type>& type) const {
  return Type::Equal(type);
}

std::string PointerType::ToString() const {
  return element_type_->ToString() + "*";
}

std::shared_ptr<Type> PointerType::GetElementType() const {
  return element_type_;
}
bool PointerType::Compatible(const std::shared_ptr<Type>& other) const {
  return Type::Compatible(other);
}
bool PointerType::Equal(const std::shared_ptr<Type>& type) const {
  return Type::Equal(type);
}

std::shared_ptr<ArrayType> ArrayType::Get(std::shared_ptr<Type> contained_type,
                                          std::size_t num_elements) {
  return std::shared_ptr<ArrayType>(
      new ArrayType{contained_type, num_elements});
}

std::string ArrayType::ToString() const {
  if (contained_type_->IsArrayTy()) {
    auto str{contained_type_->ToString()};
    auto begin{str.find_last_of('[')};
    auto end{std::size(str) - 2};
    auto num_str{str.substr(begin + 1, end - begin)};
    str.replace(begin + 1, end - begin, std::to_string(num_elements_));
    return str + '[' + num_str + ']';
  } else {
    return contained_type_->ToString() + '[' + std::to_string(num_elements_) +
           ']';
  }
}

bool ArrayType::Compatible(const std::shared_ptr<Type>& other) const {
  return Type::Compatible(other);
}

bool ArrayType::Equal(const std::shared_ptr<Type>& type) const {
  return Type::Equal(type);
}

std::string StructType::GetName() const { return name_; }

std::int32_t StructType::GetNumElements() const { return std::size(members_); }

std::shared_ptr<Type> StructType::GetElementType(std::int32_t i) const {
  return members_[i]->GetType();
}

std::string StructType::ToString() const {
  std::string s(is_struct_ ? "struct" : "union");
  s += "{";

  for (const auto& member : members_) {
    s += member->GetType()->ToString() + ";";
  }

  return s + "}";
}

bool StructType::Compatible(const std::shared_ptr<Type>& other) const {
  return Type::Compatible(other);
}

bool StructType::Equal(const std::shared_ptr<Type>& type) const {
  return Type::Equal(type);
}

void StructType::AddMember(std::shared_ptr<Object> member) {
  auto offset{MakeAlign(offset_, member->GetAlign())};
  member->SetOffset(offset);

  members_.push_back(member);
  scope_->InsertNormal(member->GetName(), member);

  align_ = std::max(align_, member->GetAlign());

  if (is_struct_) {
    offset_ = offset + member->GetType()->Width();
    width_ = MakeAlign(offset, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, member->GetType()->Width());
    width_ = MakeAlign(width_, align_);
  }
}

void StructType::MergeAnony(std::shared_ptr<Object> anony) {
  // 匿名 struct / union
  auto annoy_type{dynamic_cast<StructType*>(anony->GetType().get())};
  auto offset = MakeAlign(offset_, anony->GetAlign());

  for (auto& kv : *annoy_type->scope_) {
    auto name{kv.first};

    if (auto member{std::dynamic_pointer_cast<Object>(kv.second)}; !member) {
      continue;
    } else {
      member->SetOffset(offset + member->GetOffset());

      if (GetMember(name)) {
        Error(member->GetToken(), "duplicated member: {}", name);
      }

      scope_->InsertNormal(name, member);
    }
  }

  anony->SetOffset(offset);
  members_.push_back(anony);

  align_ = std::max(align_, anony->GetAlign());

  if (is_struct_) {
    offset_ = offset + annoy_type->Width();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, annoy_type->Width());
    width_ = MakeAlign(width_, align_);
  }
}

std::shared_ptr<Object> StructType::GetMember(const std::string& name) const {
  auto iter{scope_->FindNormalInCurrScope(name)};
  if (iter == nullptr) {
    return nullptr;
  } else {
    return std::dynamic_pointer_cast<Object>(iter);
  }
}

}  // namespace kcc
