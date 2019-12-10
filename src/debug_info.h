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
  void EmitLocalVar(const Declaration* decl);
  void EmitGlobalVar(const Declaration* decl);
  llvm::DISubprogram* EmitFuncStart(const FuncDef* func_def);

  llvm::DIType* GetBuiltinType(Type* type);
  llvm::DIType* GetPointerType(Type* type, llvm::DIFile* unit);
  llvm::DIType* GetArrayType(Type* type, llvm::DIFile* unit);
  llvm::DISubroutineType* CreateFunctionType(Type* type, llvm::DIFile* unit);
  llvm::DIType* CreateStructType(Type* type, llvm::DIFile* unit,
                                 const Location& loc);
  llvm::DIType* GetType(Type* type, llvm::DIFile* unit,
                        const Location& loc = Location{});

 private:
  llvm::DICompileUnit* cu_;

  std::vector<llvm::DIScope*> lexical_blocks_;
};

}  // namespace kcc
