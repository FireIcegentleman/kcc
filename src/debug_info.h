//
// Created by kaiser on 2019/12/10.
//

#pragma once

#include <vector>

#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Value.h>

#include "ast.h"
#include "location.h"
#include "type.h"

namespace kcc {

inline llvm::DIBuilder* DBuilder;

class DebugInfo {
 public:
  llvm::DICompileUnit* GetCu() const;
  void SetCu(llvm::DICompileUnit* cu);

  void PushLexicalBlock(llvm::DIScope* scope);
  void PopLexicalBlock();

  void EmitLocation(const AstNode* ast);

  llvm::DISubroutineType* CreateFunctionType(Type* func_type,
                                             llvm::DIFile* unit);
  static llvm::DIType* GetBuiltinType(Type* type);
  llvm::DIType* GetType(Type* type, llvm::DIFile* unit);

 private:
  llvm::DICompileUnit* cu_;

  std::vector<llvm::DIScope*> lexical_blocks_;
};

}  // namespace kcc
