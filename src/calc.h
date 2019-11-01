//
// Created by kaiser on 2019/11/2.
//

#ifndef KCC_SRC_CALC_H_
#define KCC_SRC_CALC_H_

#include <memory>

#include "ast.h"
#include "visitor.h"

namespace kcc {

template <typename T>
class Calculation : public Visitor {
 public:
  T Calc(std::shared_ptr<Expr> expr);
  void Visit(const AstNode&) override {}

 private:
};

}  // namespace kcc

#endif  // KCC_SRC_CALC_H_
