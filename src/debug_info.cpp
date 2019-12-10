//
// Created by kaiser on 2019/12/10.
//

#include "debug_info.h"

#include <cassert>
#include <filesystem>

#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/Module.h>

#include "llvm_common.h"
#include "scope.h"
#include "util.h"

namespace kcc {

DebugInfo::DebugInfo(const std::string& file_name) {
  optimize_ = OptimizationLevel != OptLevel::kO0;

  builder_ = new llvm::DIBuilder{*Module};

  std::filesystem::path path{file_name};
  path /= std::filesystem::current_path();
  file_ = builder_->createFile(path.filename().string(),
                               path.parent_path().string());

  cu_ = builder_->createCompileUnit(
      llvm::dwarf::DW_LANG_C99, file_, "kcc " KCC_VERSION, optimize_, "", 0, "",
      llvm::DICompileUnit::DebugEmissionKind::FullDebug, 0, true, false,
      llvm::DICompileUnit::DebugNameTableKind::None);

  Module->addModuleFlag(llvm::Module::Warning, "Dwarf Version",
                        llvm::dwarf::DWARF_VERSION);
  Module->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                        llvm::DEBUG_METADATA_VERSION);
}

void DebugInfo::Finalize() {
  assert(std::empty(lexical_blocks_));
  assert(subprogram_ == nullptr);
  assert(arg_index_ == 0);

  builder_->finalize();
}

void DebugInfo::EmitLocation(const AstNode* node) {
  if (!node) {
    return Builder.SetCurrentDebugLocation(llvm::DebugLoc{});
  }

  // 该作用域既可以是 compile-unit level
  // 也可以是最近的封闭词法块(如当前函数)
  llvm::DIScope* scope;
  if (std::empty(lexical_blocks_)) {
    scope = cu_;
  } else {
    scope = lexical_blocks_.back();
  }

  auto loc{node->GetLoc()};
  Builder.SetCurrentDebugLocation(
      llvm::DebugLoc::get(loc.GetRow(), loc.GetColumn(), scope));
}

void DebugInfo::EmitFuncStart(const FuncDef* node) {
  assert(node != nullptr);

  auto func_type{node->GetFuncType()};
  auto func_name{func_type->FuncGetName()};

  auto line_no{node->GetLoc().GetRow()};

  subprogram_ = builder_->createFunction(
      file_, func_name, func_name, file_, line_no,
      GetFunctionType(func_type, file_), line_no, llvm::DINode::FlagPrototyped,
      llvm::DISubprogram::SPFlagDefinition);

  lexical_blocks_.push_back(subprogram_);

  auto func{Module->getFunction(func_name)};
  assert(func != nullptr);
  func->setSubprogram(subprogram_);
}

void DebugInfo::EmitFuncEnd() {
  subprogram_ = nullptr;
  arg_index_ = 0;
  lexical_blocks_.pop_back();
}

void DebugInfo::EmitParamVar(const std::string& name, Type* type,
                             llvm::AllocaInst* ptr, const Location& loc) {
  assert(subprogram_ != nullptr);

  auto line_no{loc.GetRow()};
  auto param{builder_->createParameterVariable(subprogram_, name, ++arg_index_,
                                               file_, line_no,
                                               GetType(type, file_), true)};

  builder_->insertDeclare(ptr, param, builder_->createExpression(),
                          llvm::DebugLoc::get(line_no, 0, subprogram_),
                          Builder.GetInsertBlock());
}

void DebugInfo::EmitLocalVar(const Declaration* decl) {
  assert(decl && decl->IsObjDecl());

  llvm::DIScope* scope{lexical_blocks_.back()};
  auto ident{decl->GetIdent()};
  auto loc{decl->GetLoc()};
  auto ptr{decl->GetIdent()->ToObjectExpr()->GetLocalPtr()};

  auto var{builder_->createAutoVariable(
      scope, ident->GetName(), file_, loc.GetRow(),
      GetType(ident->GetType(), file_), optimize_)};

  builder_->insertDeclare(
      ptr, var, builder_->createExpression(),
      llvm::DebugLoc::get(loc.GetRow(), loc.GetColumn(), scope),
      Builder.GetInsertBlock());
}

void DebugInfo::EmitGlobalVar(const Declaration* decl) {
  assert(decl != nullptr);

  auto ptr{decl->GetIdent()->ToObjectExpr()->GetGlobalPtr()};
  auto ident{decl->GetIdent()};
  auto name{ident->GetName()};
  auto loc{decl->GetLoc()};

  auto info{builder_->createGlobalVariableExpression(
      cu_, name, name, file_, loc.GetRow(),
      GetType(ident->GetType(), file_, loc), ptr->hasInternalLinkage())};

  ptr->addDebugInfo(info);
}

llvm::DIType* DebugInfo::GetType(Type* type, llvm::DIFile* unit,
                                 const Location& loc) {
  if (type->IsPointerTy()) {
    return GetPointerType(type, unit);
  } else if (type->IsArrayTy()) {
    return GetArrayType(type, unit);
  } else if (type->IsStructOrUnionTy()) {
    return GetStructType(type, unit, loc);
  } else if (type->IsFunctionTy()) {
    return GetFunctionType(type, unit);
  } else {
    return GetBuiltinType(type);
  }
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

  return builder_->createBasicType(type->ToString(), type->GetWidth() * 8,
                                   encoding);
}

llvm::DIType* DebugInfo::GetPointerType(Type* type, llvm::DIFile* unit) {
  auto pointee_type{GetType(type->PointerGetElementType().GetType(), unit)};
  auto size_in_bit{type->GetWidth() * 8};
  auto align_in_bit{type->GetAlign() * 8};

  return builder_->createPointerType(pointee_type, size_in_bit, align_in_bit);
}

llvm::DIType* DebugInfo::GetArrayType(Type* type, llvm::DIFile* unit) {
  std::int32_t size{}, align{};

  if (type->IsComplete()) {
    size = type->GetWidth();
    align = type->GetAlign();
  } else {
    align = type->ArrayGetElementType()->GetAlign();
  }

  // 下标范围
  llvm::SmallVector<llvm::Metadata*, 8> subscripts;
  QualType qual_type{type};
  while (qual_type->IsArrayTy()) {
    subscripts.push_back(
        builder_->getOrCreateSubrange(0, type->ArrayGetNumElements()));
    qual_type = qual_type->ArrayGetElementType();
  }

  return builder_->createArrayType(
      size, align * 8, GetType(type->ArrayGetElementType().GetType(), unit),
      builder_->getOrCreateArray(subscripts));
}

llvm::DIType* DebugInfo::GetStructType(Type* type, llvm::DIFile* file,
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

  auto fwd_type{builder_->createReplaceableCompositeType(tag, name, scope, file,
                                                         loc.GetRow())};
  if (!type->IsComplete()) {
    return fwd_type;
  }

  llvm::SmallVector<llvm::Metadata*, 16> ele_types;
  for (const auto& [name, ident] : *type->StructGetScope()) {
    auto member_type{GetType(ident->GetType(), file, ident->GetLoc())};
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

    member_type = builder_->createMemberType(
        scope, name, file, line, size, align,
        ident->ToObjectExpr()->GetOffset(), llvm::DINode::DIFlags::FlagPublic,
        member_type);

    ele_types.push_back(member_type);
  }

  auto real_type{builder_->createStructType(
      scope, name, file, loc.GetRow(), type->GetWidth() * 8,
      type->GetAlign() * 8, llvm::DINode::DIFlags::FlagZero, nullptr,
      builder_->getOrCreateArray(ele_types))};

  return real_type;
}

llvm::DISubroutineType* DebugInfo::GetFunctionType(Type* type,
                                                   llvm::DIFile* file) {
  llvm::SmallVector<llvm::Metadata*, 8> ele_types;

  ele_types.push_back(GetType(type->FuncGetReturnType().GetType(), file));

  for (const auto& item : type->FuncGetParams()) {
    ele_types.push_back(GetType(item->GetType(), file));
  }

  return builder_->createSubroutineType(
      builder_->getOrCreateTypeArray(ele_types));
}

}  // namespace kcc
