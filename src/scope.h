//
// Created by kaiser on 2019/11/1.
//

#ifndef KCC_SRC_SCOPE_H_
#define KCC_SRC_SCOPE_H_

#include <map>
#include <memory>

namespace kcc {

enum ScopeType { kBlock, kFile, kFunc, kFuncProto };

class Scope {
 public:
 private:
  std::shared_ptr<Scope> parent_;
  enum ScopeType type_;
};

}  // namespace kcc

#endif  // KCC_SRC_SCOPE_H_
