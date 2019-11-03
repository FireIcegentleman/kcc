//
// Created by kaiser on 2019/11/1.
//

#ifndef KCC_SRC_SCOPE_H_
#define KCC_SRC_SCOPE_H_

#include <map>
#include <memory>

#include "ast.h"

namespace kcc {

enum ScopeType { kBlock, kFile, kFunc, kFuncProto };

class Scope {
 public:
  Scope(std::shared_ptr<Scope> parent, enum ScopeType type)
      : parent_{parent}, type_{type} {}

  void InsertTag(const std::string &name, std::shared_ptr<Identifier> ident);
  void InsertNormal(const std::string &name, std::shared_ptr<Identifier> ident);
  std::shared_ptr<Identifier> FindTag(const std::string &name);
  std::shared_ptr<Identifier> FindNormal(const std::string &name);
  std::shared_ptr<Identifier> FindTagInCurrScope(const std::string &name);
  std::shared_ptr<Identifier> FindNormalInCurrScope(const std::string &name);
  auto begin() { return std::begin(normal_); }
  auto end() { return std::end(normal_); }
  std::map<std::string, std::shared_ptr<Identifier>> AllTagInCurrScope() const;
  std::shared_ptr<Scope> GetParent() { return parent_; }

 private:
  std::shared_ptr<Scope> parent_;
  enum ScopeType type_;
  // struct / union / enum 的名字
  std::map<std::string, std::shared_ptr<Identifier>> tags_;
  // 函数 / 对象 / typedef名 / 枚举常量
  std::map<std::string, std::shared_ptr<Identifier>> normal_;
};

}  // namespace kcc

#endif  // KCC_SRC_SCOPE_H_
