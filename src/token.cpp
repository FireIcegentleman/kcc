//
// Created by kaiser on 2019/10/30.
//

#include "token.h"

#include <fmt/core.h>

namespace kcc {

bool Token::TagIs(Tag tag) const { return tag_ == tag; }

void Token::SetTag(Tag tag) { tag_ = tag; }

std::string Token::GetStr() const { return str_; }

void Token::SetStr(const std::string& str) { str_ = str; }

SourceLocation Token::GetLoc() const { return location_; }

void Token::SetSourceLocation(const SourceLocation& location) {
  location_ = location;
}

std::string Token::ToString() const {
  std::string loc("<" + location_.file_name + ":" +
                  std::to_string(location_.row) + ":" +
                  std::to_string(location_.column) + ">");

  return fmt::format("{:<25}str: {:<25}loc: {:<25}", kcc::ToString(tag_), str_,
                     loc);
}

bool Token::IsTypeSpecQual() const {
  return tag_ == Tag::kVoid || tag_ == Tag::kChar || tag_ == Tag::kShort ||
         tag_ == Tag::kInt || tag_ == Tag::kLong || tag_ == Tag::kFloat ||
         tag_ == Tag::kDouble || tag_ == Tag::kSigned ||
         tag_ == Tag::kUnsigned || tag_ == Tag::kBool ||
         tag_ == Tag::kComplex || tag_ == Tag::kAtomic ||
         tag_ == Tag::kStruct || tag_ == Tag::kUnion || tag_ == Tag::kEnum ||
         tag_ == Tag::kConst || tag_ == Tag::kRestrict ||
         tag_ == Tag::kVolatile || tag_ == Tag::kAtomic;
}

bool Token::IsIdentifier() const { return tag_ == Tag::kIdentifier; }

bool Token::IsStringLiteral() const { return tag_ == Tag::kStringLiteral; }

bool Token::IsConstant() const {
  return IsIntegerConstant() || IsFloatConstant() || IsCharacterConstant();
}

bool Token::IsIntegerConstant() const { return tag_ == Tag::kIntegerConstant; }

bool Token::IsFloatConstant() const { return tag_ == Tag::kFloatingConstant; }

bool Token::IsCharacterConstant() const {
  return tag_ == Tag::kCharacterConstant;
}

bool Token::IsDecl() const {
  return IsTypeSpecQual() || tag_ == Tag::kAttribute || tag_ == Tag::kInline ||
         tag_ == Tag::kNoreturn || tag_ == Tag::kAlignas ||
         tag_ == Tag::kStaticAssert || tag_ == Tag::kTypedef ||
         tag_ == Tag::kExtern || tag_ == Tag::kStatic ||
         tag_ == Tag::kThreadLocal || tag_ == Tag::kAuto ||
         tag_ == Tag::kRegister;
}

}  // namespace kcc
