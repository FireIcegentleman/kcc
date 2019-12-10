//
// Created by kaiser on 2019/12/10.
//

#include "debug_info.h"

#include <cassert>

#include <llvm/ADT/SmallVector.h>

#include "llvm_common.h"

namespace kcc {

llvm::DICompileUnit* DebugInfo::GetCu() const { return cu_; }

void DebugInfo::SetCu(llvm::DICompileUnit* cu) { cu_ = cu; }

void DebugInfo::PushLexicalBlock(llvm::DIScope* scope) {
  lexical_blocks_.push_back(scope);
}

void DebugInfo::PopLexicalBlock() { lexical_blocks_.pop_back(); }

void DebugInfo::EmitLocation(const AstNode* ast) {
  if (!ast) {
    return;
  }

  llvm::DIScope* scope;
  if (std::empty(lexical_blocks_)) {
    scope = cu_;
  } else {
    scope = lexical_blocks_.back();
  }

  auto loc{ast->GetLoc()};
  Builder.SetCurrentDebugLocation(
      llvm::DebugLoc::get(loc.GetRow(), loc.GetColumn(), scope));
}

llvm::DISubroutineType* DebugInfo::CreateFunctionType(Type* func_type,
                                                      llvm::DIFile* unit) {
  llvm::SmallVector<llvm::Metadata*, 8> ele_types;

  ele_types.push_back(GetType(func_type->FuncGetReturnType().GetType(), unit));

  for (const auto& item : func_type->FuncGetParams()) {
    ele_types.push_back(GetType(item->GetType(), unit));
  }

  return DBuilder->createSubroutineType(
      DBuilder->getOrCreateTypeArray(ele_types));
}

llvm::DIType* DebugInfo::GetBuiltinType(Type* type) {
  std::int32_t encoding{};

  if (type->IsVoidTy()) {
    return nullptr;
  } else if (type->IsCharacterTy()) {
    encoding = type->IsUnsigned() ? llvm::dwarf::DW_ATE_unsigned_char
                                  : llvm::dwarf::DW_ATE_signed_char;
  } else if (type->IsBoolTy()) {
    encoding = llvm::dwarf::DW_ATE_boolean;
  } else if (type->IsFloatPointTy()) {
    encoding = llvm::dwarf::DW_ATE_float;
  } else if (type->IsIntegerTy()) {
    encoding = type->IsUnsigned() ? llvm::dwarf::DW_ATE_unsigned
                                  : llvm::dwarf::DW_ATE_signed;
  } else {
    assert(false);
  }

  return DBuilder->createBasicType(type->ToString(), type->GetWidth() * 8,
                                   encoding);
}

llvm::DIType* DebugInfo::GetType(Type* type, llvm::DIFile* unit) {
  (void)unit;
  return GetBuiltinType(type);
}

}  // namespace kcc
