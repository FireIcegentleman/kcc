//
// Created by kaiser on 2019/10/31.
//

#include "ast.h"

#include "visitor.h"

namespace kcc {

#define ACCEPT(ClassName) \
  void ClassName::Accept(Visitor& visitor) const { visitor.Visit(*this); }

ACCEPT(AstNode)
ACCEPT(TranslationUnit)
ACCEPT(BinaryOpExpr)
ACCEPT(UnaryOpExpr)
ACCEPT(ConditionOpExpr)

}  // namespace kcc
