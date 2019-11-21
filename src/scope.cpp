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

void Scope::InsertUsual(IdentifierExpr* ident) {
  InsertUsual(ident->GetName(), ident);
}

void Scope::InsertTag(const std::string& name, IdentifierExpr* ident) {
  tags_[name] = ident;
}

void Scope::InsertUsual(const std::string& name, IdentifierExpr* ident) {
  usual_[name] = ident;
}

IdentifierExpr* Scope::FindTag(const std::string& name) {
  auto iter{tags_.find(name)};
  if (iter != std::end(tags_)) {
    return iter->second;
  }

  if (type_ == kFile || parent_ == nullptr) {
    return nullptr;
  } else {
    return parent_->FindTag(name);
  }
}

IdentifierExpr* Scope::FindUsual(const std::string& name) {
  auto iter{usual_.find(name)};
  if (iter != std::end(usual_)) {
    return iter->second;
  }

  if (type_ == kFile || parent_ == nullptr) {
    return nullptr;
  } else {
    return parent_->FindUsual(name);
  }
}

IdentifierExpr* Scope::FindTagInCurrScope(const std::string& name) {
  auto iter{tags_.find(name)};
  return iter == std::end(tags_) ? nullptr : iter->second;
}

IdentifierExpr* Scope::FindUsualInCurrScope(const std::string& name) {
  auto iter{usual_.find(name)};
  return iter == std::end(usual_) ? nullptr : iter->second;
}

IdentifierExpr* Scope::FindTag(const Token& tok) {
  return FindTag(tok.GetIdentifier());
}

IdentifierExpr* Scope::FindUsual(const Token& tok) {
  return FindUsual(tok.GetIdentifier());
}

IdentifierExpr* Scope::FindTagInCurrScope(const Token& tok) {
  return FindTagInCurrScope(tok.GetIdentifier());
}

IdentifierExpr* Scope::FindUsualInCurrScope(const Token& tok) {
  return FindUsualInCurrScope(tok.GetIdentifier());
}

std::unordered_map<std::string, IdentifierExpr*> Scope::AllTagInCurrScope()
    const {
  return tags_;
}

Scope* Scope::GetParent() { return parent_; }

bool Scope::IsFileScope() const { return type_ == kFile; }

Scope::Scope(Scope* parent, enum ScopeType type)
    : parent_{parent}, type_{type} {}

}  // namespace kcc
