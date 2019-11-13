//
// Created by kaiser on 2019/11/1.
//

#include "scope.h"

#include "memory_pool.h"

#ifdef DEV
#include <iostream>
#endif

namespace kcc {

Scope* Scope::Get(Scope* parent, enum ScopeType type) {
  return new (ScopePool.Allocate()) Scope{parent, type};
}

#ifdef DEV
void Scope::PrintCurrScope() const {
  std::cout << "\n\n---------------------------------------\n";
  std::cout << "func / object / typedef / enumerator :\n";
  for (const auto& [name, obj] : normal_) {
    std::cout << "name: " << name << "\t\t\t";
    std::cout << "type: " << obj->GetType()->ToString() << '\n';
  }
  std::cout << "---------------------------------------" << std::endl;
  std::cout << "struct / union / enum :\n";
  for (const auto& [name, obj] : tags_) {
    std::cout << "name: " << name << "\t\t\t";
    std::cout << "type: " << obj->GetType()->ToString();

    if (obj->GetType()->IsStructOrUnionTy()) {
      std::cout << "\t\t\tmember: " << obj->GetType()->StructGetNumMembers();
      for (const auto& item : obj->GetType()->StructGetMembers()) {
        std::cout << item->GetName() << ' ';
      }
    }
    std::cout << '\n';
  }
  std::cout << "---------------------------------------" << std::endl;
}
#endif

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
