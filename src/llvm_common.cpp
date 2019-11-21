//
// Created by kaiser on 2019/11/17.
//

#include "llvm_common.h"

#include <assert.h>

namespace kcc {

llvm::Constant *GetConstantZero(llvm::Type *type) {
  if (type->isIntegerTy()) {
    return llvm::ConstantInt::get(type, 0);
  } else if (type->isFloatingPointTy()) {
    return llvm::ConstantFP::get(type, 0.0);
  } else if (type->isPointerTy()) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
  } else if (type->isAggregateType()) {
    return llvm::ConstantAggregateZero::get(type);
  } else {
    assert(false);
    return nullptr;
  }
}

}  // namespace kcc
