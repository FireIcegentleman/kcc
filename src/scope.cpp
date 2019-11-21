//
// Created by kaiser on 2019/11/1.
//

#include "scope.h"

#include "memory_pool.h"

namespace kcc {

Scope* Scope::Get(Scope* parent, enum ScopeType type) {
  return new (ScopePool.Allocate()) Scope{parent, type};
}

void Scope::InsertTag(IdentifierExpr* ident) {
  InsertTag(ident->GetName(), ident);
}

void Scope::InsertNormal(IdentifierExpr* ident) {
  InsertNormal(ident->GetName(), ident);
}

void Scope::InsertTag(const std::string& name, IdentifierExpr* ident) {
  tags_[name] = ident;
}

void Scope::InsertNormal(const std::string& name, IdentifierExpr* ident) {
  normal_[name] = ident;
}

IdentifierExpr* Scope::FindTag(const std::string& name) {
  auto iter{tags_.find(name)};
  if (iter != std::end(tags_)) {
    return iter->second;
  }

  if (type_ == kFile || parent_ == nullptr) {
    return nullptr;
  }
  return parent_->FindTag(name);
}

IdentifierExpr* Scope::FindNormal(const std::string& name) {
  auto iter{normal_.find(name)};
  if (iter != std::end(normal_)) {
    return iter->second;
  }

  if (type_ == kFile || parent_ == nullptr) {
    return nullptr;
  }
  return parent_->FindNormal(name);
}

IdentifierExpr* Scope::FindTagInCurrScope(const std::string& name) {
  auto iter{tags_.find(name)};
  return iter == std::end(tags_) ? nullptr : iter->second;
}

IdentifierExpr* Scope::FindNormalInCurrScope(const std::string& name) {
  auto iter{normal_.find(name)};
  return iter == std::end(normal_) ? nullptr : iter->second;
}

IdentifierExpr* Scope::FindTag(const Token& tok) {
  return FindTag(tok.GetIdentifier());
}

IdentifierExpr* Scope::FindNormal(const Token& tok) {
  return FindNormal(tok.GetIdentifier());
}

IdentifierExpr* Scope::FindTagInCurrScope(const Token& tok) {
  return FindTagInCurrScope(tok.GetIdentifier());
}

IdentifierExpr* Scope::FindNormalInCurrScope(const Token& tok) {
  return FindNormalInCurrScope(tok.GetIdentifier());
}

std::unordered_map<std::string, IdentifierExpr*> Scope::AllTagInCurrScope()
    const {
  return tags_;
}

Scope* Scope::GetParent() { return parent_; }

bool Scope::IsFileScope() const { return type_ == kFile; }

}  // namespace kcc
