//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_VISITOR_H_
#define KCC_SRC_VISITOR_H_

#include "ast.h"

namespace kcc {

class Visitor {
 public:
  virtual ~Visitor() = default;

  virtual void Visit(const AstNode &node) = 0;
};

}  // namespace kcc

#endif  // KCC_SRC_VISITOR_H_
