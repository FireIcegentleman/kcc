//
// Created by kaiser on 2019/11/2.
//

#include "code_gen.h"

#include <llvm/ADT/Optional.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include "error.h"

namespace kcc {

// 拥有许多 LLVM 核心数据结构, 如类型和常量值表
llvm::LLVMContext Context;
// 一个辅助对象, 跟踪当前位置并且可以插入 LLVM 指令
llvm::IRBuilder<> Builder{Context};
// 包含函数和全局变量, 它拥有生成的所有 IR 的内存
auto Module{std::make_unique<llvm::Module>("main", Context)};

llvm::DataLayout DataLayout{Module.get()};

std::unique_ptr<llvm::TargetMachine> TargetMachine;

CodeGen::CodeGen(const std::string& file_name) {
  Module = std::make_unique<llvm::Module>(file_name, Context);
  DataLayout = llvm::DataLayout{Module.get()};

  // 初始化
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();

  // 获取当前计算机的目标三元组
  auto target_triple{llvm::sys::getDefaultTargetTriple()};

  std::string error;
  auto target{llvm::TargetRegistry::lookupTarget(target_triple, error)};

  if (!target) {
    Error(error);
  }

  // 使用通用CPU, 生成位置无关目标文件
  std::string cpu("generic");
  std::string features;
  llvm::TargetOptions opt;
  llvm::Optional<llvm::Reloc::Model> rm{llvm::Reloc::Model::PIC_};
  TargetMachine = std::unique_ptr<llvm::TargetMachine>{
      target->createTargetMachine(target_triple, cpu, features, opt, rm)};

  // 配置模块以指定目标机器和数据布局, 这不是必需的, 但有利于优化
  Module->setDataLayout(TargetMachine->createDataLayout());
  Module->setTargetTriple(target_triple);
}

void CodeGen::GenCode(const std::shared_ptr<TranslationUnit>& root) {
  root->Accept(*this);
}

/*
 * ++ --
 * + - ~
 * !
 * * &
 */
void CodeGen::Visit(const UnaryOpExpr& node) {
  node.expr_->Accept(*this);

  switch (node.op_) {
    case Tag::kPlusPlus:

      break;
    case Tag::kPostfixPlusPlus:
      break;
    case Tag::kMinusMinus:
      break;
    case Tag::kPostfixMinusMinus:
      break;
    case Tag::kPlus:
      break;
    case Tag::kMinus:
      break;
    case Tag::kTilde:
      break;
    case Tag::kExclaim:
      break;
    case Tag::kStar:
      break;
    case Tag::kAmp:
    default:
      assert(false);
  }
}

void CodeGen::Visit(const BinaryOpExpr& node) {
  node.lhs_->Accept(*this);
  auto lhs{result_};
  node.rhs_->Accept(*this);
  auto rhs{result_};

  bool is_unsigned{node.GetType()->IsUnsigned()};

  switch (node.op_) {
    case Tag::kPlus:
      result_ = AddOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kMinus:
      result_ = SubOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kStar:
      result_ = MulOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kSlash:
      result_ = DivOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kPercent:
      result_ = ModOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kPipe:
      result_ = OrOp(lhs, rhs);
      break;
    case Tag::kAmp:
      result_ = AndOp(lhs, rhs);
      break;
    case Tag::kCaret:
      result_ = XorOp(lhs, rhs);
      break;
    case Tag::kLessLess:
      result_ = ShlOp(lhs, rhs);
      break;
    case Tag::kGreaterGreater:
      result_ = ShrOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kEqualEqual:
      result_ = EqualOp(lhs, rhs);
      break;
    case Tag::kExclaimEqual:
      result_ = NotEqualOp(lhs, rhs);
      break;
    case Tag::kLess:
      result_ = LessOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kLessEqual:
      result_ = LessEqualOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kGreater:
      result_ = GreaterOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kGreaterEqual:
      result_ = GreaterEqualOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kComma:
      result_ = rhs;
      break;
    default:
      assert(false);
  }
}

void CodeGen::Visit(const ConditionOpExpr&) {}

void CodeGen::Visit(const TypeCastExpr& node) { node.expr_->Accept(*this); }

// 常量用 ConstantFP / ConstantInt 类表示
// 在 LLVM IR 中, 常量都是唯一且共享的
void CodeGen::Visit(const Constant& node) {
  auto type{node.GetType()->GetLLVMType()};

  if (type->isIntegerTy()) {
    result_ = llvm::ConstantInt::get(
        Context, llvm::APInt(type->getIntegerBitWidth(), node.integer_val_,
                             !node.GetType()->IsUnsigned()));
  } else if (type->isFloatingPointTy()) {
    result_ =
        llvm::ConstantFP::get(Context, llvm::APFloat(node.float_point_val_));
  } else {
    result_ = Builder.CreateGlobalStringPtr(node.str_val_);
  }
}

void CodeGen::Visit(const Enumerator&) {}

void CodeGen::Visit(const CompoundStmt& node) {
  for (auto& item : node.stmts_) {
    item->Accept(*this);
  }
}

void CodeGen::Visit(const IfStmt&) {}

void CodeGen::Visit(const ReturnStmt&) {}

void CodeGen::Visit(const LabelStmt&) {}

void CodeGen::Visit(const FuncCallExpr& node) {
  node.callee_->Accept(*this);
  auto callee{result_};

  std::vector<llvm::Value*> values;
  for (auto& item : node.args_) {
    item->Accept(*this);
    values.push_back(result_);
  }

  Builder.CreateCall(callee, values);
}

void CodeGen::Visit(const Identifier& node) {
  auto type{node.GetType()};
  assert(type->IsFunctionTy());

  auto func = Module->getFunction(node.GetName());
  if (!func) {
    func = llvm::Function::Create(
        llvm::cast<llvm::FunctionType>(type->GetLLVMType()),
        node.GetLinkage() == kInternal ? llvm::Function::InternalLinkage
                                       : llvm::Function::ExternalLinkage,
        node.GetName(), Module.get());
  }

  result_ = func;
}

void CodeGen::Visit(const Object& node) {
  if (!node.in_global_) {
    result_ = Builder.CreateAlignedLoad(node.local_ptr_, node.GetAlign());
  } else {
    assert(false);
  }
}

void CodeGen::Visit(const TranslationUnit& node) {
  for (auto& item : node.ext_decls_) {
    item->Accept(*this);
  }
}

void CodeGen::Visit(const Declaration& node) {
  if (node.IsObj()) {
    auto obj{std::dynamic_pointer_cast<Object>(node.ident_)};
    auto type{node.ident_->GetType()};

    if (!obj->in_global_) {
      obj->local_ptr_ = CreateEntryBlockAlloca(
          Builder.GetInsertBlock()->getParent(), type->GetLLVMType(),
          obj->GetAlign(), obj->GetName());
    }
  }
}

void CodeGen::Visit(const FuncDef& node) {
  auto func_name{node.GetName()};
  auto type{node.GetFuncType()};
  auto func{Module->getFunction(func_name)};

  if (!func) {
    func = llvm::Function::Create(
        llvm::cast<llvm::FunctionType>(type->GetLLVMType()),
        node.GetIdent()->GetLinkage() == kInternal
            ? llvm::Function::InternalLinkage
            : llvm::Function::ExternalLinkage,
        node.GetIdent()->GetName(), Module.get());
  }

  auto entry{llvm::BasicBlock::Create(Context, "", func)};
  Builder.SetInsertPoint(entry);

  node.body_->Accept(*this);

  Builder.CreateRet(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)));

  // 验证生成的代码, 检查一致性
  llvm::verifyFunction(*func);
}

void CodeGen::Visit(const ExprStmt& node) {
  if (node.expr_) {
    node.expr_->Accept(*this);
  }
}

void CodeGen::Visit(const WhileStmt&) {}

void CodeGen::Visit(const DoWhileStmt&) {}

void CodeGen::Visit(const ForStmt&) {}

void CodeGen::Visit(const CaseStmt&) {}

void CodeGen::Visit(const DefaultStmt&) {}

void CodeGen::Visit(const SwitchStmt&) {}

void CodeGen::Visit(const BreakStmt&) {}

void CodeGen::Visit(const ContinueStmt&) {}

void CodeGen::Visit(const GotoStmt&) {}

llvm::AllocaInst* CodeGen::CreateEntryBlockAlloca(llvm::Function* parent,
                                                  llvm::Type* type,
                                                  std::int32_t align,
                                                  const std::string& name) {
  // 指向入口块开始的位置
  llvm::IRBuilder<> temp{&parent->getEntryBlock(),
                         parent->getEntryBlock().begin()};

  auto ptr{temp.CreateAlloca(type, nullptr, name)};
  ptr->setAlignment(align);

  return ptr;
}

llvm::Value* CodeGen::AddOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return Builder.CreateAdd(lhs, rhs);
    } else {
      return Builder.CreateNSWAdd(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return Builder.CreateFAdd(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    // 进行地址计算, 第二个参数是偏移量列表
    return llvm::GetElementPtrInst::CreateInBounds(lhs, {rhs}, "",
                                                   Builder.GetInsertBlock());
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::SubOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return Builder.CreateSub(lhs, rhs);
    } else {
      return Builder.CreateNSWSub(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return Builder.CreateFSub(lhs, rhs);
  } else if (IsPointerTy(lhs) && IsIntegerTy(rhs)) {
    return llvm::GetElementPtrInst::CreateInBounds(
        lhs, {Builder.CreateNeg(rhs)}, "", Builder.GetInsertBlock());
  } else if (IsPointerTy(lhs) && IsPointerTy(rhs)) {
    auto type{lhs->getType()->getPointerElementType()};

    lhs = CastTo(lhs, Builder.getInt64Ty(), false);
    rhs = CastTo(rhs, Builder.getInt64Ty(), false);

    auto value{SubOp(lhs, rhs, false)};
    return DivOp(value, Builder.getInt64(DataLayout.getTypeAllocSize(type)),
                 false);
  } else {
    assert(false);
    return nullptr;
  }
}

bool CodeGen::IsArithmeticTy(llvm::Value* value) const {
  return IsIntegerTy(value) || IsFloatingPointTy(value);
}

bool CodeGen::IsPointerTy(llvm::Value* value) const {
  return value->getType()->isPointerTy();
}

bool CodeGen::IsIntegerTy(llvm::Value* value) const {
  return value->getType()->isIntegerTy();
}

bool CodeGen::IsFloatingPointTy(llvm::Value* value) const {
  return value->getType()->isFloatingPointTy();
}

llvm::Value* CodeGen::MulOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return Builder.CreateMul(lhs, rhs);
    } else {
      return Builder.CreateNSWMul(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return Builder.CreateFMul(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::DivOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return Builder.CreateUDiv(lhs, rhs);
    } else {
      return Builder.CreateSDiv(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return Builder.CreateFDiv(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::CastTo(llvm::Value* value, llvm::Type* to,
                             bool is_unsigned) {
  if (to->isIntegerTy(1)) {
    return CastToBool(value);
  }

  if (IsIntegerTy(value) && to->isIntegerTy()) {
    if (is_unsigned) {
      return Builder.CreateZExtOrTrunc(value, to);
    } else {
      return Builder.CreateSExtOrTrunc(value, to);
    }
  } else if (IsIntegerTy(value) && to->isFloatingPointTy()) {
    if (is_unsigned) {
      return Builder.CreateUIToFP(value, to);
    } else {
      return Builder.CreateSIToFP(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isFloatingPointTy()) {
    if (is_unsigned) {
      return Builder.CreateFPToUI(value, to);
    } else {
      return Builder.CreateFPToSI(value, to);
    }
  } else if (FloatPointRank(value->getType()) > FloatPointRank(to)) {
    return Builder.CreateFPTrunc(value, to);
  } else if (FloatPointRank(value->getType()) < FloatPointRank(to)) {
    return Builder.CreateFPExt(value, to);
  } else if (to->isVoidTy()) {
    return value;
  } else {
    assert(false);
  }
}

llvm::Value* CodeGen::ModOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (is_unsigned) {
    return Builder.CreateURem(lhs, rhs);
  } else {
    return Builder.CreateSRem(lhs, rhs);
  }
}

llvm::Value* CodeGen::OrOp(llvm::Value* lhs, llvm::Value* rhs) {
  return Builder.CreateOr(lhs, rhs);
}

llvm::Value* CodeGen::AndOp(llvm::Value* lhs, llvm::Value* rhs) {
  return Builder.CreateAnd(lhs, rhs);
}

llvm::Value* CodeGen::XorOp(llvm::Value* lhs, llvm::Value* rhs) {
  return Builder.CreateXor(lhs, rhs);
}

llvm::Value* CodeGen::ShlOp(llvm::Value* lhs, llvm::Value* rhs) {
  return Builder.CreateShl(lhs, rhs);
}

llvm::Value* CodeGen::ShrOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (is_unsigned) {
    return Builder.CreateLShr(lhs, rhs);
  } else {
    return Builder.CreateAShr(lhs, rhs);
  }
}

llvm::Value* CodeGen::LessEqualOp(llvm::Value* lhs, llvm::Value* rhs,
                                  bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{Builder.CreateICmpULE(lhs, rhs)};
      return CastTo(value, Builder.getInt32Ty(), false);
    } else {
      auto value{Builder.CreateICmpSLE(lhs, rhs)};
      return CastTo(value, Builder.getInt32Ty(), false);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOLE(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpULE(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::LessOp(llvm::Value* lhs, llvm::Value* rhs,
                             bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{Builder.CreateICmpULT(lhs, rhs)};
      return CastTo(value, Builder.getInt32Ty(), false);
    } else {
      auto value{Builder.CreateICmpSLT(lhs, rhs)};
      return CastTo(value, Builder.getInt32Ty(), false);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOLT(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpULT(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::GreaterEqualOp(llvm::Value* lhs, llvm::Value* rhs,
                                     bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{Builder.CreateICmpUGE(lhs, rhs)};
      return CastTo(value, Builder.getInt32Ty(), false);
    } else {
      auto value{Builder.CreateICmpSGE(lhs, rhs)};
      return CastTo(value, Builder.getInt32Ty(), false);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOGE(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpUGE(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::GreaterOp(llvm::Value* lhs, llvm::Value* rhs,
                                bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{Builder.CreateICmpUGT(lhs, rhs)};
      return CastTo(value, Builder.getInt32Ty(), false);
    } else {
      auto value{Builder.CreateICmpSGT(lhs, rhs)};
      return CastTo(value, Builder.getInt32Ty(), false);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOGT(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpUGT(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::EqualOp(llvm::Value* lhs, llvm::Value* rhs) {
  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpEQ(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOEQ(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
  }
}

llvm::Value* CodeGen::NotEqualOp(llvm::Value* lhs, llvm::Value* rhs) {
  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpNE(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpONE(lhs, rhs)};
    return CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
  }
}

llvm::Value* CodeGen::CastToBool(llvm::Value* value) {
  if (IsIntegerTy(value) || IsPointerTy(value)) {
    return Builder.CreateICmpNE(value, GetZero(value->getType()));
  } else if (IsFloatingPointTy(value)) {
    return Builder.CreateFCmpONE(value, GetZero(value->getType()));
  } else {
    assert(false);
  }
}

bool CodeGen::IsFloatTy(llvm::Value* value) const {
  return value->getType()->isFloatTy();
}

bool CodeGen::IsDoubleTy(llvm::Value* value) const {
  return value->getType()->isDoubleTy();
}

bool CodeGen::IsLongDoubleTy(llvm::Value* value) const {
  return value->getType()->isX86_FP80Ty();
}

llvm::Value* CodeGen::GetZero(llvm::Type* type) {
  if (type->isIntegerTy()) {
    return llvm::ConstantInt::get(Context,
                                  llvm::APInt(type->getIntegerBitWidth(), 0));
  } else if (type->isFloatTy()) {
    return llvm::ConstantFP::get(Context, llvm::APFloat(0.0f));
  } else if (type->isDoubleTy()) {
    return llvm::ConstantFP::get(Context, llvm::APFloat(0.0));
  } else if (type->isX86_FP80Ty()) {
    return llvm::ConstantFP::get(llvm::Type::getX86_FP80Ty(Context),
                                 llvm::APFloat(0.0));
  } else if (type->isPointerTy()) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
  } else {
    assert(false);
  }
}

std::int32_t CodeGen::FloatPointRank(llvm::Type* type) const {
  if (type->isFloatTy()) {
    return 0;
  } else if (type->isDoubleTy()) {
    return 1;
  } else if (type->isX86_FP80Ty()) {
    return 2;
  } else {
    assert(false);
  }
}

}  // namespace kcc
