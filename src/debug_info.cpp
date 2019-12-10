//
// Created by kaiser on 2019/12/10.
//

#include "debug_info.h"

#include <cassert>

#include <llvm/ADT/SmallVector.h>

#include "llvm_common.h"
#include "scope.h"
#include "util.h"

namespace kcc {

llvm::DICompileUnit* DebugInfo::GetCu() const { return cu_; }

void DebugInfo::SetCu(llvm::DICompileUnit* cu) { cu_ = cu; }

void DebugInfo::PushLexicalBlock(llvm::DIScope* scope) {
  lexical_blocks_.push_back(scope);
}

void DebugInfo::PopLexicalBlock() { lexical_blocks_.pop_back(); }

void DebugInfo::EmitLocation(const AstNode* ast) {
  if (Debug) {
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
}

llvm::DISubroutineType* DebugInfo::CreateFunctionType(Type* type,
                                                      llvm::DIFile* unit) {
  llvm::SmallVector<llvm::Metadata*, 8> ele_types;

  ele_types.push_back(GetType(type->FuncGetReturnType().GetType(), unit));

  for (const auto& item : type->FuncGetParams()) {
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

llvm::DIType* DebugInfo::GetPointerType(Type* type, llvm::DIFile* unit) {
  return DBuilder->createPointerType(
      GetType(type->PointerGetElementType().GetType(), unit),
      type->GetWidth() * 8);
}

llvm::DIType* DebugInfo::GetType(Type* type, llvm::DIFile* unit,
                                 const Location& loc) {
  if (type->IsPointerTy()) {
    return GetPointerType(type, unit);
  } else if (type->IsArrayTy()) {
    return GetArrayType(type, unit);
  } else if (type->IsStructOrUnionTy()) {
    return CreateStructType(type, unit, loc);
  } else if (type->IsFunctionTy()) {
    return CreateFunctionType(type, unit);
  } else {
    return GetBuiltinType(type);
  }
}

llvm::DIType* DebugInfo::CreateStructType(Type* type, llvm::DIFile* unit,
                                          const Location& loc) {
  std::int32_t tag{};
  if (type->IsStructTy()) {
    tag = llvm::dwarf::DW_TAG_structure_type;
  } else if (type->IsUnionTy()) {
    tag = llvm::dwarf::DW_TAG_union_type;
  } else {
    assert(false);
  }

  std::string name{type->StructGetName()};
  llvm::DIScope* scope;
  if (std::empty(lexical_blocks_)) {
    scope = cu_;
  } else {
    scope = lexical_blocks_.back();
  }

  auto fwd_type{DBuilder->createReplaceableCompositeType(tag, name, scope, unit,
                                                         loc.GetRow())};
  if (!type->IsComplete()) {
    return fwd_type;
  }

  llvm::SmallVector<llvm::Metadata*, 16> ele_types;
  for (const auto& [name, ident] : *type->StructGetScope()) {
    auto member_type{GetType(ident->GetType(), unit, ident->GetLoc())};
    auto member_name{name};

    if (std::empty(name)) {
      continue;
    }

    auto line{ident->GetLoc().GetRow()};
    std::int32_t size{};
    std::int32_t align{};

    if (!(ident->GetType()->IsArrayTy() && !ident->GetType()->IsComplete())) {
      size = ident->GetType()->GetWidth() * 8;
      align = ident->GetType()->GetAlign();
    }

    member_type = DBuilder->createMemberType(
        scope, name, unit, line, size, align,
        ident->ToObjectExpr()->GetOffset(), llvm::DINode::DIFlags::FlagPublic,
        member_type);

    ele_types.push_back(member_type);
  }

  auto real_type{DBuilder->createStructType(
      scope, name, unit, loc.GetRow(), type->GetWidth() * 8,
      type->GetAlign() * 8, llvm::DINode::DIFlags::FlagZero, nullptr,
      DBuilder->getOrCreateArray(ele_types))};

  return real_type;
}

llvm::DIType* DebugInfo::GetArrayType(Type* type, llvm::DIFile* unit) {
  std::int32_t size{}, align{};

  if (type->IsComplete()) {
    size = type->GetWidth();
    align = type->GetAlign();
  } else {
    align = type->ArrayGetElementType()->GetAlign();
  }

  llvm::SmallVector<llvm::Metadata*, 8> subscripts;
  QualType sub_type{type};
  while (sub_type->IsArrayTy()) {
    subscripts.push_back(
        DBuilder->getOrCreateSubrange(0, type->ArrayGetNumElements()));
    sub_type = sub_type->ArrayGetElementType();
  }

  return DBuilder->createArrayType(
      size, align, GetType(type->ArrayGetElementType().GetType(), unit),
      DBuilder->getOrCreateArray(subscripts));
}

void DebugInfo::EmitLocalVar(const Declaration* decl) {
  if (Debug) {
    llvm::DIScope* scope{lexical_blocks_.back()};

    auto var{DBuilder->createAutoVariable(
        scope, decl->GetIdent()->GetName(), lexical_blocks_.back()->getFile(),
        decl->GetLoc().GetRow(),
        GetType(decl->GetIdent()->GetType(),
                lexical_blocks_.back()->getFile()))};

    DBuilder->insertDeclare(
        decl->GetIdent()->ToObjectExpr()->GetLocalPtr(), var,
        DBuilder->createExpression(),
        llvm::DebugLoc::get(decl->GetLoc().GetRow(), decl->GetLoc().GetColumn(),
                            scope),
        Builder.GetInsertBlock());
  }
}

void DebugInfo::EmitGlobalVar(const Declaration* decl) {
  if (Debug) {
    llvm::DIScope* scope{cu_};
    auto var{decl->GetIdent()->ToObjectExpr()->GetGlobalPtr()};
    var->addDebugInfo(DBuilder->createGlobalVariableExpression(
        scope, decl->GetIdent()->GetName(), "", cu_->getFile(),
        decl->GetLoc().GetRow(),
        GetType(decl->GetIdent()->GetType(), cu_->getFile(), decl->GetLoc()),
        var->hasInternalLinkage()));
  }
}

llvm::DISubprogram* DebugInfo::EmitFuncStart(const FuncDef* func_def) {
  llvm::DISubprogram* sp{};

  if (Debug) {
    auto unit{DBuilder->createFile(cu_->getFilename(), cu_->getDirectory())};
    llvm::DIScope* file_context{unit};
    auto line_no{func_def->GetLoc().GetRow()};
    sp = DBuilder->createFunction(
        file_context, func_def->GetFuncType()->FuncGetName(), "", unit, line_no,
        CreateFunctionType(func_def->GetFuncType(), unit), line_no,
        llvm::DINode::FlagPrototyped, llvm::DISubprogram::SPFlagDefinition);
    lexical_blocks_.push_back(sp);
  }

  return sp;
}

}  // namespace kcc
