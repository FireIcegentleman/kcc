//
// Created by kaiser on 2019/10/31.
//

#include "type.h"

#include <cassert>

#include "ast.h"
#include "scope.h"

namespace kcc {

/*
 * QualType
 */
Type& QualType::operator*() { return *type_; }

const Type& QualType::operator*() const { return *type_; }

std::shared_ptr<Type> QualType::operator->() { return type_; }

const std::shared_ptr<Type> QualType::operator->() const { return type_; }

std::shared_ptr<Type> QualType::GetType() { return type_; }

const std::shared_ptr<Type> QualType::GetType() const { return type_; }

std::uint32_t QualType::GetTypeQual() const { return type_qual_; }

bool QualType::IsConst() const { return type_qual_ & kConst; }

bool QualType::IsRestrict() const { return type_qual_ & kRestrict; }

bool operator==(QualType lhs, QualType rhs) { return lhs.type_ == rhs.type_; }

bool operator!=(QualType lhs, QualType rhs) { return !(lhs == rhs); }

/*
 * Type
 */
QualType Type::MayCast(QualType type, bool in_proto) {
  if (type->IsFunctionTy()) {
    return QualType{type->GetPointerTo()};
  } else if (type->IsArrayTy()) {
    auto ret{PointerType::Get(type->ArrayGetElementType())};
    return QualType{ret, in_proto ? 0U : kConst};
  } else {
    return type;
  }
}

bool Type::IsComplete() const { return complete_; }

void Type::SetComplete(bool complete) const { complete_ = complete; }

bool Type::IsUnsigned() const {
  assert(IsIntegerTy());

  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kUnsigned);
}

bool Type::IsVoidTy() const {
  auto p{dynamic_cast<const ArithmeticType*>(this)};
  return p && (p->type_spec_ & kVoid);
}

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
  return p && (p->type_spec_ & (kLong | kDouble));
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
  return p && p->IsStructTy();
}

bool Type::IsUnionTy() const {
  auto p{dynamic_cast<const StructType*>(this)};
  return p && !p->IsStructTy();
}

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

bool Type::IsArithmeticTy() const { return IsIntegerTy() || IsFloatPointTy(); }

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

std::shared_ptr<PointerType> Type::GetPointerTo() {
  return PointerType::Get(QualType{shared_from_this()});
}

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

bool Type::StructHasName() const {
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

std::vector<std::shared_ptr<Object>> Type::StructGetMembers() {
  return dynamic_cast<StructType*>(this)->GetMembers();
}

std::shared_ptr<Object> Type::StructGetMember(const std::string& name) const {
  return dynamic_cast<const StructType*>(this)->GetMember(name);
}

QualType Type::StructGetMemberType(std::int32_t i) const {
  return dynamic_cast<const StructType*>(this)->GetMemberType(i);
}

std::shared_ptr<Scope> Type::StructGetScope() {
  return dynamic_cast<StructType*>(this)->GetScope();
}

void Type::StructAddMember(std::shared_ptr<Object> member) {
  dynamic_cast<StructType*>(this)->AddMember(member);
}

void Type::StructMergeAnonymous(std::shared_ptr<Object> anonymous) {
  dynamic_cast<StructType*>(this)->MergeAnonymous(anonymous);
}

std::int32_t Type::StructGetOffset() const {
  return dynamic_cast<const StructType*>(this)->GetOffset();
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

std::vector<std::shared_ptr<Object>> Type::FuncGetParams() const {
  return dynamic_cast<const FunctionType*>(this)->GetParams();
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

Type::Type(bool complete) : complete_{complete} {}

/*
 * ArithmeticType
 */
std::shared_ptr<ArithmeticType> ArithmeticType::Get(std::uint32_t type_spec) {
  return std::shared_ptr<ArithmeticType>(new ArithmeticType{type_spec});
}

std::shared_ptr<Type> ArithmeticType::IntegerPromote(
    std::shared_ptr<Type> type) {
  assert(type->IsIntegerTy());

  auto int_type{ArithmeticType::Get(kInt)};
  if (type->ArithmeticRank() < int_type->Rank()) {
    return int_type;
  } else {
    return type;
  }
}

std::shared_ptr<Type> ArithmeticType::MaxType(std::shared_ptr<Type> lhs,
                                              std::shared_ptr<Type> rhs) {
  assert(lhs->IsArithmeticTy() && rhs->IsArithmeticTy());

  if (!lhs->IsIntegerTy() || !rhs->IsIntegerTy()) {
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
      return 8;
    case kShort:
    case kShort | kUnsigned:
      return 16;
    case kInt:
    case kInt | kUnsigned:
      return 32;
    case kLong:
    case kLong | kUnsigned:
      return 64;
    case kLongLong:
    case kLongLong | kUnsigned:
      return 64;
    case kFloat:
      return 32;
    case kDouble:
      return 64;
    case kDouble | kLong:
      return 128;
    default:
      assert(false);
  }
}

std::int32_t ArithmeticType::GetAlign() const { return GetWidth(); }

std::string ArithmeticType::ToString() const {
  if (IsIntegerTy()) {
    return std::string(IsUnsigned() ? "u" : "") + "int" +
           std::to_string(GetWidth());
  } else if (IsFloatTy()) {
    return "float";
  } else if (IsDoubleTy()) {
    return "double";
  } else if (IsLongDoubleTy()) {
    return "long double";
  } else {
    assert(false);
  }
}

// 它们是同一类型（同名或由 typedef 引入的别名）
bool ArithmeticType::Compatible(const std::shared_ptr<Type>& other) const {
  return Equal(other);
}

bool ArithmeticType::Equal(const std::shared_ptr<Type>& type) const {
  if (type->IsArithmeticTy()) {
    return type_spec_ ==
           std::dynamic_pointer_cast<ArithmeticType>(type)->type_spec_;
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
      return std::numeric_limits<std::uint32_t>::max();
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
  }
}

void ArithmeticType::SetUnsigned() { type_spec_ |= kUnsigned; }

ArithmeticType::ArithmeticType(std::uint32_t type_spec) : Type{true} {
  if (type_spec == kSigned) {
    type_spec = kInt;
  } else if (type_spec == kUnsigned) {
    type_spec |= kInt;
  }

  type_spec &= ~kSigned;

  if ((type_spec & kShort) || (type_spec & kLong) || (type_spec & kLongLong)) {
    type_spec &= ~kInt;
  }

  type_spec_ = type_spec;
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
  }
}

/*
 * PointerType
 */
std::shared_ptr<PointerType> PointerType::Get(QualType element_type) {
  return std::shared_ptr<PointerType>{new PointerType{element_type}};
}

std::int32_t PointerType::GetWidth() const { return 8; }

std::int32_t PointerType::GetAlign() const { return GetWidth(); }

std::string PointerType::ToString() const {
  return element_type_->ToString() + "*";
}

// 它们是指针类型，并指向兼容类型
bool PointerType::Compatible(const std::shared_ptr<Type>& other) const {
  if (other->IsPointerTy()) {
    return element_type_->Compatible(
        std::dynamic_pointer_cast<PointerType>(other)->element_type_.GetType());
  } else {
    return false;
  }
}

bool PointerType::Equal(const std::shared_ptr<Type>& other) const {
  if (other->IsPointerTy()) {
    return element_type_->Equal(
        std::dynamic_pointer_cast<PointerType>(other)->element_type_.GetType());
  } else {
    return false;
  }
}

QualType PointerType::GetElementType() const { return element_type_; }

PointerType::PointerType(QualType element_type)
    : Type{true}, element_type_{element_type} {}

/*
 * ArrayType
 */
std::shared_ptr<ArrayType> ArrayType::Get(QualType contained_type,
                                          std::size_t num_elements) {
  return std::shared_ptr<ArrayType>(
      new ArrayType{contained_type, num_elements});
}

std::int32_t ArrayType::GetWidth() const {
  assert(num_elements_ != 0);
  return contained_type_->GetWidth() * num_elements_;
}

std::int32_t ArrayType::GetAlign() const { return GetWidth(); }

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

// 其元素类型兼容，且
// 若都拥有常量大小，则大小相同。
// 注意：未知边界数组与任何兼容元素类型的数组兼容。
// VLA 与任何兼容元素类型的数组兼容。 (C99 起)
bool ArrayType::Compatible(const std::shared_ptr<Type>& other) const {
  if (other->IsArrayTy()) {
    auto other_arr{std::dynamic_pointer_cast<ArrayType>(other)};
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

bool ArrayType::Equal(const std::shared_ptr<Type>& other) const {
  if (other->IsArrayTy()) {
    auto other_arr{std::dynamic_pointer_cast<ArrayType>(other)};
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
  if (num_elements > 0) {
    SetComplete(true);
  }
  num_elements_ = num_elements;
}

std::size_t ArrayType::GetNumElements() const { return num_elements_; }

QualType ArrayType::GetElementType() const { return contained_type_; }

ArrayType::ArrayType(QualType contained_type, std::size_t num_elements)
    : Type{num_elements > 0},
      contained_type_{contained_type},
      num_elements_{num_elements} {}

/*
 * StructType
 */
std::shared_ptr<StructType> StructType::Get(bool is_struct, bool has_name,
                                            std::shared_ptr<Scope> parent) {
  return std::shared_ptr<StructType>{
      new StructType{is_struct, has_name, parent}};
}

std::int32_t StructType::GetWidth() const { return width_; }

std::int32_t StructType::GetAlign() const { return align_; }

std::string StructType::ToString() const {
  std::string s(is_struct_ ? "struct" : "union");
  s += "{";

  // FIXME 当某成员类型为指向该 struct / union 的指针
  for (const auto& member : members_) {
    s += member->GetType()->ToString() + ";";
  }

  return s + "}";
}

// (C99)若一者以标签声明，则另一者必须以同一标签声明。
// 若它们都是完整类型，则其成员必须在数量上准确对应，以兼容类型声明，并拥有匹配的名称。
// 另外，若它们都是枚举，则对应成员亦必须拥有相同值。
// 另外，若它们是结构体或联合体，则
// 对应的元素必须以同一顺序声明（仅结构体）
// 对应的位域必须有相同宽度。
bool StructType::Compatible(const std::shared_ptr<Type>& other) const {
  if (other->IsStructTy()) {
    auto other_struct{std::dynamic_pointer_cast<StructType>(other)};
    if (HasName() && other_struct->HasName()) {
      if (name_ != other_struct->name_) {
        return false;
      }
    }

    if (IsComplete() && other_struct->IsComplete()) {
      if (GetNumMembers() != other_struct->GetNumMembers()) {
        return false;
      }
      auto iter{std::end(other_struct->members_)};
      for (const auto& member : members_) {
        if (!member->GetType()->Compatible((*iter)->GetType().GetType())) {
          return false;
        }

        if (member->GetName() != (*iter)->GetName()) {
          return false;
        }
      }
    }

    return true;
  } else {
    return false;
  }
}

bool StructType::Equal(const std::shared_ptr<Type>& other) const {
  if (other->IsStructTy()) {
    auto other_struct{std::dynamic_pointer_cast<StructType>(other)};
    if (HasName() && other_struct->HasName()) {
      if (name_ != other_struct->name_) {
        return false;
      }
    }

    if (IsComplete() && other_struct->IsComplete()) {
      if (GetNumMembers() != other_struct->GetNumMembers()) {
        return false;
      }
      auto iter{std::end(other_struct->members_)};
      for (const auto& member : members_) {
        if (!member->GetType()->Equal((*iter)->GetType().GetType())) {
          return false;
        }

        if (member->GetName() != (*iter)->GetName()) {
          return false;
        }
      }
    }

    return true;
  } else {
    return false;
  }
}

bool StructType::IsStruct() const { return is_struct_; }

bool StructType::HasName() const { return has_name_; }

void StructType::SetName(const std::string& name) { name_ = name; }

std::string StructType::GetName() const { return name_; }

std::int32_t StructType::GetNumMembers() const { return std::size(members_); }

std::vector<std::shared_ptr<Object>> StructType::GetMembers() {
  return members_;
}

std::shared_ptr<Object> StructType::GetMember(const std::string& name) const {
  auto iter{scope_->FindNormalInCurrScope(name)};
  if (iter == nullptr) {
    return nullptr;
  } else {
    return std::dynamic_pointer_cast<Object>(iter);
  }
}

QualType StructType::GetMemberType(std::int32_t i) const {
  return members_[i]->GetType();
}

std::shared_ptr<Scope> StructType::GetScope() { return scope_; }

std::int32_t StructType::GetOffset() const { return offset_; }

void StructType::AddMember(std::shared_ptr<Object> member) {
  auto offset{MakeAlign(offset_, member->GetAlign())};
  member->SetOffset(offset);

  members_.push_back(member);
  scope_->InsertNormal(member->GetName(), member);

  align_ = std::max(align_, member->GetAlign());

  if (is_struct_) {
    offset_ = offset + member->GetType()->GetWidth();
    width_ = MakeAlign(offset, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, member->GetType()->GetWidth());
    width_ = MakeAlign(width_, align_);
  }
}

void StructType::MergeAnonymous(std::shared_ptr<Object> anonymous) {
  // 匿名 struct / union
  auto annoy_type{
      std::dynamic_pointer_cast<StructType>(anonymous->GetType().GetType())};
  auto offset = MakeAlign(offset_, anonymous->GetAlign());

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

  anonymous->SetOffset(offset);
  members_.push_back(anonymous);

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

StructType::StructType(bool is_struct, bool has_name,
                       std::shared_ptr<Scope> parent)
    : Type{false}, is_struct_{is_struct}, has_name_{has_name}, scope_{parent} {}

std::int32_t StructType::MakeAlign(std::int32_t offset, std::int32_t align) {
  if (offset % align == 0) {
    return offset;
  } else {
    return offset + align - (offset % align);
  }
}

/*
 * FunctionType
 */
std::shared_ptr<FunctionType> FunctionType::Get(
    QualType return_type, std::vector<std::shared_ptr<Object>> params,
    bool is_var_args) {
  return std::shared_ptr<FunctionType>{
      new FunctionType{return_type, params, is_var_args}};
}

std::int32_t FunctionType::GetWidth() const { assert(false); }

std::int32_t FunctionType::GetAlign() const { assert(false); }

std::string FunctionType::ToString() const {
  std::string s(return_type_->ToString() + "(");

  for (const auto& param : params_) {
    s += param->GetType()->ToString() + ", ";
  }

  if (is_var_args_) {
    s += "...)";
    return s;
  } else {
    if (std::size(params_) != 0) {
      s.pop_back();
      s.pop_back();
    } else {
      s += "void";
    }

    return s + ")";
  }
}

// 其返回类型兼容
// 它们都使用参数列表，参数数量（包括省略号的使用）相同，而其对应参数，在应用数组到指针和函数到指针类型调整，
// 及剥除顶层限定符后，拥有相同类型
// 一个是旧式（无参数）定义，另一个有参数列表，参数列表不使用省略号，而每个参数（在函数参数类型调整后）
// 都与默认参数提升后的对应旧式参数兼容
// 一个是旧式（无参数）声明，另一个拥有参数列表，参数列表不使用省略号，而所有参数（在函数参数类型调整后）
// 不受默认参数提升影响
bool FunctionType::Compatible(const std::shared_ptr<Type>& other) const {
  if (other->IsFunctionTy()) {
    auto other_func{std::dynamic_pointer_cast<FunctionType>(other)};
    if (!return_type_->Compatible(other_func->return_type_.GetType())) {
      return false;
    }
    if (GetNumParams() != other_func->GetNumParams()) {
      return false;
    }

    auto iter{std::begin(other_func->params_)};
    for (const auto& param : params_) {
      if (!param->GetType()->Equal((*iter)->GetType().GetType())) {
        return false;
      }
      ++iter;
    }

    return true;
  } else {
    return false;
  }
}

bool FunctionType::Equal(const std::shared_ptr<Type>& other) const {
  if (other->IsFunctionTy()) {
    auto other_func{std::dynamic_pointer_cast<FunctionType>(other)};
    if (!return_type_->Equal(other_func->return_type_.GetType())) {
      return false;
    }
    if (GetNumParams() != other_func->GetNumParams()) {
      return false;
    }

    auto iter{std::begin(other_func->params_)};
    for (const auto& param : params_) {
      if (!param->GetType()->Equal((*iter)->GetType().GetType())) {
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
  return params_[i]->GetType();
}

std::vector<std::shared_ptr<Object>> FunctionType::GetParams() const {
  return params_;
}

void FunctionType::SetFuncSpec(std::uint32_t func_spec) {
  func_spec_ = func_spec;
}

bool FunctionType::IsInline() const { return func_spec_ & kInline; }

bool FunctionType::IsNoreturn() const { return func_spec_ & kNoreturn; }

FunctionType::FunctionType(QualType return_type,
                           std::vector<std::shared_ptr<Object>> param,
                           bool is_var_args)
    : Type{false},
      is_var_args_{is_var_args},
      return_type_{return_type},
      params_{param} {}
}  // namespace kcc
