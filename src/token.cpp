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

SourceLocation Token::GetSourceLocation() const { return location_; }

void Token::SetSourceLocation(const SourceLocation& location) {
  location_ = location;
}

std::string Token::ToString() const {
  return fmt::format("{:<30}str: {:<30}", kcc::ToString(tag_), str_);
}

}  // namespace kcc
