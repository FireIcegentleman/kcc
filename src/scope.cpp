//
// Created by kaiser on 2019/11/1.
//

#include "scope.h"

namespace kcc {

void Scope::InsertTag(const std::string& name,
                      std::shared_ptr<Identifier> ident) {
  tags_[name] = ident;
}

void Scope::InsertNormal(const std::string& name,
                         std::shared_ptr<Identifier> ident) {
  normal_[name] = ident;
}

std::shared_ptr<Identifier> Scope::FindTag(const std::string& name) {
  auto iter{tags_.find(name)};
  if (iter != std::end(tags_)) {
    return iter->second;
  }

  if (type_ == kFile || parent_ == nullptr) {
    return nullptr;
  }
  return parent_->FindTag(name);
}

std::shared_ptr<Identifier> Scope::FindNormal(const std::string& name) {
  auto iter{normal_.find(name)};
  if (iter != std::end(normal_)) {
    return iter->second;
  }

  if (type_ == kFile || parent_ == nullptr) {
    return nullptr;
  }
  return parent_->FindTag(name);
}

std::shared_ptr<Identifier> Scope::FindTagInCurrScope(const std::string& name) {
  auto iter{tags_.find(name)};
  return iter == std::end(tags_) ? nullptr : iter->second;
}

std::shared_ptr<Identifier> Scope::FindNormalInCurrScope(
    const std::string& name) {
  auto iter{normal_.find(name)};
  return iter == std::end(normal_) ? nullptr : iter->second;
}

std::map<std::string, std::shared_ptr<Identifier>> Scope::AllTagInCurrScope()
    const {
  return tags_;
}

}  // namespace kcc
