//
// Created by kaiser on 2019/11/1.
//

#include "scope.h"

namespace kcc {

void Scope::PrintCurrScope() const {
  for (const auto& item : normal_) {
    std::cout << "func / object / typedef / enumerator :\n";
    std::cout << item.first << ' ';
    std::cout << item.second->GetType()->ToString() << '\n';
  }
  std::cout << "struct / union / enum :\n";
  for (const auto& item : tags_) {
    std::cout << item.first << ' ';
    std::cout << item.second->GetType()->ToString() << '\n';
  }
}

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
  return parent_->FindNormal(name);
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

std::shared_ptr<Identifier> Scope::FindTag(const Token& tok) {
  return FindTag(tok.GetStr());
}

std::shared_ptr<Identifier> Scope::FindNormal(const Token& tok) {
  return FindNormal(tok.GetStr());
}

std::shared_ptr<Identifier> Scope::FindTagInCurrScope(const Token& tok) {
  return FindTagInCurrScope(tok.GetStr());
}

std::shared_ptr<Identifier> Scope::FindNormalInCurrScope(const Token& tok) {
  return FindNormalInCurrScope(tok.GetStr());
}

std::map<std::string, std::shared_ptr<Identifier>> Scope::AllTagInCurrScope()
    const {
  return tags_;
}

std::shared_ptr<Scope> Scope::GetParent() { return parent_; }

bool Scope::IsFileScope() const { return type_ == kFile; }

}  // namespace kcc
