//
// Created by kaiser on 2019/11/2.
//

#include "code_gen.h"

#include <cassert>
#include <memory>
#include <vector>

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/Optional.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include "error.h"
#include "util.h"

namespace kcc {

CodeGen::CodeGen(const std::string&) {
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
  DataLayout = TargetMachine->createDataLayout();
  Module->setDataLayout(DataLayout);
  Module->setTargetTriple(target_triple);
}

void CodeGen::GenCode(const TranslationUnit* root) {
  root->Accept(*this);

#ifndef DEV
  if (llvm::verifyModule(*Module, &llvm::errs())) {
    Error("module is broken");
  }
#endif

  assert(std::size(continue_stack_) == 0);
  assert(std::size(break_stack_) == 0);
  assert(std::size(has_br_or_return_) == 0);
  assert(std::size(switch_insts_) == 0);
  assert(std::size(switch_after_) == 0);
}

/*
 * ++ --
 * + - ~
 * !
 * * &
 */
void CodeGen::Visit(const UnaryOpExpr& node) {
  auto is_unsigned{node.expr_->GetType()->IsUnsigned()};

  switch (node.op_) {
    case Tag::kPlusPlus:
      result_ = IncOrDec(*node.expr_, true, false);
      break;
    case Tag::kPostfixPlusPlus:
      result_ = IncOrDec(*node.expr_, true, true);
      break;
    case Tag::kMinusMinus:
      result_ = IncOrDec(*node.expr_, false, false);
      break;
    case Tag::kPostfixMinusMinus:
      result_ = IncOrDec(*node.expr_, false, true);
      break;
    case Tag::kPlus:
      node.expr_->Accept(*this);
      break;
    case Tag::kMinus:
      node.expr_->Accept(*this);
      result_ = NegOp(result_, is_unsigned);
      break;
    case Tag::kTilde:
      node.expr_->Accept(*this);
      result_ = Builder.CreateNot(result_);
      break;
    case Tag::kExclaim:
      node.expr_->Accept(*this);
      result_ = LogicNotOp(result_);
      break;
    case Tag::kStar: {
      auto binary{dynamic_cast<BinaryOpExpr*>(node.expr_)};
      if (binary && binary->op_ == Tag::kPlus) {
        // e.g. a[1]
        auto backup{load_};
        load_ = false;
        binary->lhs_->Accept(*this);
        auto lhs{result_};
        binary->rhs_->Accept(*this);
        load_ = backup;
        if (lhs->getType()->isPointerTy() &&
            lhs->getType()->getPointerElementType()->isArrayTy()) {
          result_ =
              Builder.CreateInBoundsGEP(lhs, {result_, Builder.getInt64(0)});
        } else {
          result_ = Builder.CreateInBoundsGEP(lhs, {result_});
          result_ = Builder.CreateAlignedLoad(result_, align_);
        }
      } else if (node.expr_->GetType()->IsPointerTy() &&
                 node.expr_->GetType()
                     ->PointerGetElementType()
                     ->IsFunctionTy()) {
        node.expr_->Accept(*this);
      } else {
        result_ = Builder.CreateAlignedLoad(GetPtr(node), align_);
      }
    } break;
    case Tag::kAmp:
      result_ = GetPtr(*node.expr_);
      break;
    default:
      assert(false);
  }
}

void CodeGen::Visit(const TypeCastExpr& node) {
  node.expr_->Accept(*this);
  result_ = CastTo(result_, node.to_->GetLLVMType(),
                   node.expr_->GetType()->IsUnsigned());
}

/*
 * =
 * + - * / % & | ^ << >>
 * && ||
 * == != < > <= >=
 * .
 * ,
 */
void CodeGen::Visit(const BinaryOpExpr& node) {
  switch (node.op_) {
    case Tag::kPipePipe:
      result_ = LogicOrOp(node);
      return;
    case Tag::kAmpAmp:
      result_ = LogicAndOp(node);
      return;
    case Tag::kEqual:
      result_ = AssignOp(node);
      return;
    case Tag::kPeriod: {
      auto ptr{GetPtr(node)};
      auto type{ptr->getType()->getPointerElementType()};
      if (type->isAggregateType()) {
        if (type->isStructTy() && load_) {
          result_ = Builder.CreateAlignedLoad(ptr, align_);
        } else {
          result_ = ptr;
        }
      } else {
        result_ = Builder.CreateAlignedLoad(ptr, align_);
      }
    }
      return;
    default:
      break;
  }

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

void CodeGen::Visit(const ConditionOpExpr& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto lhs_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto rhs_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  node.cond_->Accept(*this);
  result_ = CastToBool(result_);
  auto cond{result_};
  Builder.CreateCondBr(cond, lhs_block, rhs_block);

  Builder.SetInsertPoint(lhs_block);

  last_ = nullptr;
  node.lhs_->Accept(*this);
  lhs_block = last_ == nullptr ? lhs_block : last_;
  last_ = nullptr;

  auto lhs_value{result_};
  Builder.CreateBr(after_block);

  Builder.SetInsertPoint(rhs_block);

  last_ = nullptr;
  node.rhs_->Accept(*this);
  rhs_block = last_ == nullptr ? rhs_block : last_;
  last_ = nullptr;

  auto rhs_value{result_};
  Builder.CreateBr(after_block);

  Builder.SetInsertPoint(after_block);
  last_ = after_block;

  if (lhs_value->getType()->isVoidTy() || rhs_value->getType()->isVoidTy()) {
    return;
  } else {
    auto phi{Builder.CreatePHI(lhs_value->getType(), 2)};
    phi->addIncoming(lhs_value, lhs_block);
    phi->addIncoming(rhs_value, rhs_block);

    result_ = phi;
  }
}

// LLVM 默认使用本机 C 调用约定
void CodeGen::Visit(const FuncCallExpr& node) {
  if (MayCallBuiltinFunc(node)) {
    return;
  }

  node.callee_->Accept(*this);
  auto callee{result_};

  std::vector<llvm::Value*> values;
  load_ = true;
  for (auto& item : node.args_) {
    item->Accept(*this);
    values.push_back(result_);
  }
  load_ = false;

  result_ = Builder.CreateCall(callee, values);
}

// 常量用 ConstantFP / ConstantInt 类表示
// 在 LLVM IR 中, 常量都是唯一且共享的
void CodeGen::Visit(const ConstantExpr& node) {
  auto type{node.GetType()->GetLLVMType()};

  if (type->isIntegerTy()) {
    result_ = llvm::ConstantInt::get(
        Context, llvm::APInt(type->getIntegerBitWidth(), node.integer_val_,
                             !node.GetType()->IsUnsigned()));
  } else if (type->isFloatingPointTy()) {
    result_ = llvm::ConstantFP::get(type, node.float_point_val_);
  } else {
    assert(false);
  }
}

// 1 / 2 / 4
// 注意已经添加空字符了
void CodeGen::Visit(const StringLiteralExpr& node) {
  auto width{node.GetType()->ArrayGetElementType()->GetWidth()};
  auto size{node.GetType()->ArrayGetNumElements()};
  auto str{node.val_.c_str()};
  llvm::Constant* arr{};

  switch (width) {
    case 1: {
      std::vector<std::uint8_t> values;
      for (std::size_t i{}; i < size; ++i) {
        auto ptr{reinterpret_cast<const std::uint8_t*>(str)};
        values.push_back(static_cast<std::uint64_t>(*ptr));
        str += 1;
      }
      arr = llvm::ConstantDataArray::get(Context, values);
    } break;
    case 2: {
      std::vector<std::uint16_t> values;
      for (std::size_t i{}; i < size; ++i) {
        auto ptr{reinterpret_cast<const std::uint16_t*>(str)};
        values.push_back(static_cast<std::uint64_t>(*ptr));
        str += 2;
      }
      arr = llvm::ConstantDataArray::get(Context, values);
    } break;
    case 4: {
      std::vector<std::uint32_t> values;
      for (std::size_t i{}; i < size; ++i) {
        auto ptr{reinterpret_cast<const std::uint32_t*>(str)};
        values.push_back(static_cast<std::uint64_t>(*ptr));
        str += 4;
      }
      arr = llvm::ConstantDataArray::get(Context, values);
    } break;
    default:
      assert(false);
  }

  if (auto iter{strings_.find(arr)}; iter != std::end(strings_)) {
    result_ = iter->second;
    return;
  }

  auto global_var{new llvm::GlobalVariable(*Module, arr->getType(), true,
                                           llvm::GlobalValue::PrivateLinkage,
                                           arr, ".str")};
  global_var->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  global_var->setAlignment(width);

  auto zero{llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), 0)};
  auto ptr{llvm::ConstantExpr::getInBoundsGetElementPtr(
      global_var->getValueType(), global_var,
      llvm::ArrayRef<llvm::Constant*>{zero, zero})};

  strings_[arr] = ptr;
  result_ = ptr;
}

void CodeGen::Visit(const IdentifierExpr& node) {
  auto type{node.GetType()};
  assert(type->IsFunctionTy());

  auto name{node.GetName()};

  auto func{Module->getFunction(name)};
  if (!func) {
    func = llvm::Function::Create(
        llvm::cast<llvm::FunctionType>(type->GetLLVMType()),
        node.GetLinkage() == kInternal ? llvm::Function::InternalLinkage
                                       : llvm::Function::ExternalLinkage,
        name, Module.get());
  }

  result_ = func;
}

void CodeGen::Visit(const EnumeratorExpr& node) {
  result_ = llvm::ConstantInt::get(Builder.getInt32Ty(), node.val_);
}

void CodeGen::Visit(const ObjectExpr& node) {
  align_ = node.GetAlign();

  if (node.InGlobal() || node.IsStatic()) {
    if (node.GetType()->IsAggregateTy()) {
      if (node.GetType()->IsStructOrUnionTy() && load_) {
        result_ = Builder.CreateAlignedLoad(node.GetGlobalPtr(), align_);
      } else {
        result_ = node.GetGlobalPtr();
      }
    } else {
      result_ = Builder.CreateAlignedLoad(node.GetGlobalPtr(), align_);
    }
  } else {
    if (node.GetType()->IsAggregateTy()) {
      if (node.GetType()->IsStructOrUnionTy() && load_) {
        result_ = Builder.CreateAlignedLoad(node.GetLocalPtr(), align_);
      } else {
        result_ = node.GetLocalPtr();
      }
    } else {
      result_ = Builder.CreateAlignedLoad(node.GetLocalPtr(), node.GetAlign());
    }
  }
}

void CodeGen::Visit(const StmtExpr& node) { node.block_->Accept(*this); }

void CodeGen::Visit(const LabelStmt& node) {
  if (!node.has_goto_) {
    node.stmt_->Accept(*this);
    return;
  }

  node.label_ = llvm::BasicBlock::Create(Context, "",
                                         Builder.GetInsertBlock()->getParent());
  if (!HasBrOrReturn()) {
    Builder.CreateBr(node.label_);
  }
  Builder.SetInsertPoint(node.label_);
  labels_.push_back(node);

  has_br_or_return_.top().first = false;
  node.stmt_->Accept(*this);
}

void CodeGen::Visit(const CaseStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto case_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto inst{switch_insts_.top()};
  bool first{inst->getNumCases() == 0};

  if (!first && !case_has_break_ && !HasBrOrReturn()) {
    Builder.CreateBr(case_block);
  }
  PopBlock();
  case_has_break_ = false;

  if (node.has_range_) {
    for (auto i{node.case_value_range_.first};
         i <= node.case_value_range_.second; ++i) {
      inst->addCase(llvm::ConstantInt::get(Builder.getInt32Ty(), i),
                    case_block);
    }
  } else {
    inst->addCase(
        llvm::ConstantInt::get(Builder.getInt32Ty(), node.case_value_),
        case_block);
  }

  Builder.SetInsertPoint(case_block);
  PushBlock(nullptr, switch_after_.top());
  node.block_->Accept(*this);
}

void CodeGen::Visit(const DefaultStmt& node) {
  auto inst{switch_insts_.top()};
  auto default_block{inst->getDefaultDest()};

  if (!case_has_break_) {
    Builder.CreateBr(default_block);
  }

  Builder.SetInsertPoint(default_block);
  has_br_or_return_.push({false, false});
  node.block_->Accept(*this);
}

void CodeGen::Visit(const CompoundStmt& node) {
  for (auto& item : node.stmts_) {
    if (item) {
      item->Accept(*this);
    }
  }
}

void CodeGen::Visit(const ExprStmt& node) {
  if (node.expr_) {
    node.expr_->Accept(*this);
  }
}

void CodeGen::Visit(const IfStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto then_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  llvm::BasicBlock* else_block{};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  node.cond_->Accept(*this);
  result_ = CastToBool(result_);

  if (node.else_block_) {
    else_block = llvm::BasicBlock::Create(Context, "", parent_func);
    Builder.CreateCondBr(result_, then_block, else_block);
  } else {
    Builder.CreateCondBr(result_, then_block, after_block);
  }

  Builder.SetInsertPoint(then_block);

  has_br_or_return_.push({false, false});
  node.then_block_->Accept(*this);
  // 注意如果已经有一个无条件跳转指令了, 则不能再有一个
  if (!HasBrOrReturn()) {
    Builder.CreateBr(after_block);
  }
  has_br_or_return_.pop();

  if (node.else_block_) {
    Builder.SetInsertPoint(else_block);

    has_br_or_return_.push({false, false});
    node.else_block_->Accept(*this);
    if (!HasBrOrReturn()) {
      Builder.CreateBr(after_block);
    }
    has_br_or_return_.pop();
  }

  Builder.SetInsertPoint(after_block);
}

void CodeGen::Visit(const SwitchStmt& node) {
  if (!node.has_case_ && !node.has_default_) {
    return;
  }

  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  llvm::BasicBlock* default_block{nullptr};

  if (node.has_default_) {
    default_block = llvm::BasicBlock::Create(Context, "", parent_func);
  } else {
    default_block = after_block;
  }

  node.cond_->Accept(*this);
  auto inst{Builder.CreateSwitch(result_, default_block)};
  switch_insts_.push(inst);
  switch_after_.push(after_block);

  PushBlock(nullptr, after_block);
  node.block_->Accept(*this);

  if (!case_has_break_ && !node.has_default_) {
    Builder.CreateBr(default_block);
  } else if (node.has_default_) {
    if (!HasBrOrReturn()) {
      Builder.CreateBr(after_block);
    }
    has_br_or_return_.pop();
  }

  PopBlock();

  switch_insts_.pop();
  switch_after_.pop();

  Builder.SetInsertPoint(after_block);
}

void CodeGen::Visit(const WhileStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto cond_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(cond_block);

  Builder.SetInsertPoint(cond_block);
  node.cond_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, loop_block, after_block);

  Builder.SetInsertPoint(loop_block);

  PushBlock(cond_block, after_block);
  node.block_->Accept(*this);

  if (!HasBrOrReturn()) {
    Builder.CreateBr(cond_block);
  }
  PopBlock();

  Builder.SetInsertPoint(after_block);
}

void CodeGen::Visit(const DoWhileStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto cond_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(loop_block);

  Builder.SetInsertPoint(loop_block);
  PushBlock(cond_block, after_block);

  node.block_->Accept(*this);
  if (!HasBrOrReturn()) {
    Builder.CreateBr(cond_block);
  }

  PopBlock();

  Builder.SetInsertPoint(cond_block);
  node.cond_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, loop_block, after_block);

  Builder.SetInsertPoint(after_block);
}

void CodeGen::Visit(const ForStmt& node) {
  if (node.init_) {
    node.init_->Accept(*this);
  } else if (node.decl_) {
    node.decl_->Accept(*this);
  }

  if (!node.inc_ && !node.cond_) {
    VisitForNoIncCond(node);
  } else if (!node.inc_ && node.cond_) {
    VisitForNoInc(node);
  } else if (node.inc_ && !node.cond_) {
    VisitForNoCond(node);
  } else {
    auto parent_func{Builder.GetInsertBlock()->getParent()};
    auto cond_block{llvm::BasicBlock::Create(Context, "", parent_func)};
    auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
    auto inc_block{llvm::BasicBlock::Create(Context, "", parent_func)};
    auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

    Builder.CreateBr(cond_block);
    Builder.SetInsertPoint(cond_block);

    node.cond_->Accept(*this);
    result_ = CastToBool(result_);
    Builder.CreateCondBr(result_, loop_block, after_block);

    Builder.SetInsertPoint(loop_block);

    PushBlock(inc_block, after_block);
    node.block_->Accept(*this);

    Builder.CreateBr(inc_block);

    Builder.SetInsertPoint(inc_block);

    node.inc_->Accept(*this);
    if (!HasBrOrReturn()) {
      Builder.CreateBr(cond_block);
    }
    PopBlock();

    Builder.SetInsertPoint(after_block);
  }
}

void CodeGen::Visit(const GotoStmt& node) {
  assert(node.label_->has_goto_);

  if (!node.label_->label_) {
    goto_stmt_.push_back(
        {node.GetName(), Builder.GetInsertBlock(),
         Builder.GetInsertBlock()->getInstList().size() - alloc_count_});
  } else {
    Builder.CreateBr(node.label_->label_);
  }

  has_br_or_return_.top().first = true;
}

void CodeGen::Visit(const ContinueStmt&) {
  if (std::empty(continue_stack_) || continue_stack_.top() == nullptr) {
    Error("Cannot use continue here");
  } else {
    Builder.CreateBr(continue_stack_.top());
    has_br_or_return_.top().first = true;
  }
}

void CodeGen::Visit(const BreakStmt&) {
  if (std::empty(break_stack_) || break_stack_.top() == nullptr) {
    Error("Cannot use break here");
  } else {
    Builder.CreateBr(break_stack_.top());
    has_br_or_return_.top().first = true;
    case_has_break_ = true;
  }
}

void CodeGen::Visit(const ReturnStmt& node) {
  if (node.expr_) {
    auto back{load_};
    load_ = true;
    node.expr_->Accept(*this);
    load_ = back;
    Builder.CreateRet(result_);
  } else {
    Builder.CreateRetVoid();
  }
  has_br_or_return_.top().second = true;
}

void CodeGen::Visit(const TranslationUnit& node) {
  PushBlock(nullptr, nullptr);

  for (const auto& item : node.GetExtDecl()) {
    item->Accept(*this);
  }

  PopBlock();
}

void CodeGen::Visit(const Declaration& node) {
  // 对于函数声明, 当函数调用或者定义时处理
  if (!node.IsObjDecl()) {
    return;
  }

  if (node.IsObjDeclInGlobal()) {
    DealGlobalDecl(node);
  } else {
    DealLocaleDecl(node);
  }
}

void CodeGen::Visit(const FuncDef& node) {
  auto func_name{node.GetName()};
  auto func_type{node.GetFuncType()};
  auto func{Module->getFunction(func_name)};

  if (!func) {
    func = llvm::Function::Create(
        llvm::cast<llvm::FunctionType>(func_type->GetLLVMType()),
        node.GetLinkage() == kInternal ? llvm::Function::InternalLinkage
                                       : llvm::Function::ExternalLinkage,
        func_name, Module.get());
  }

  // TODO Attribute DSOLocal 用法
  // TODO 实现 inline
  if (node.GetLinkage() != kInternal) {
    func->setDSOLocal(true);
  }

  func->addFnAttr(llvm::Attribute::NoUnwind);
  func->addFnAttr(llvm::Attribute::StackProtectStrong);
  func->addFnAttr(llvm::Attribute::UWTable);

  if (OptimizationLevel == OptLevel::kO0) {
    func->addFnAttr(llvm::Attribute::NoInline);
    func->addFnAttr(llvm::Attribute::OptimizeNone);
  }

  auto entry{llvm::BasicBlock::Create(Context, "", func)};
  Builder.SetInsertPoint(entry);
  EnterFunc();

  auto iter{std::begin(func_type->FuncGetParams())};
  for (auto&& arg : func->args()) {
    auto ptr{CreateEntryBlockAlloca(func, arg.getType(), (*iter)->GetAlign())};
    (*iter)->SetLocalPtr(ptr);

    // 将参数的值保存到分配的内存中
    Builder.CreateAlignedStore(&arg, ptr, (*iter)->GetAlign());
    ++iter;
  }

  node.body_->Accept(*this);

  if (!HasReturn()) {
    if (func_name == "main") {
      Builder.CreateRet(Builder.getInt32(0));
    } else {
      if (!func_type->FuncGetReturnType()->IsVoidTy()) {
        Builder.CreateRet(
            GetZero(func_type->FuncGetReturnType()->GetLLVMType()));
      } else {
        Builder.CreateRetVoid();
      }
    }
  }
  ExitFunc();

  for (const auto& [name, bb, size] : goto_stmt_) {
    Builder.SetInsertPoint(bb);
    for (std::size_t i{};
         i < bb->getInstList().size() - alloc_count_ - size + 1 &&
         !bb->getInstList().empty();
         ++i) {
      bb->getInstList().back().eraseFromParent();
    }
    for (const auto& item : labels_) {
      if (name == item.GetIdent()) {
        Builder.CreateBr(item.label_);
        break;
      }
    }
  }
  alloc_count_ = 0;
  goto_stmt_.clear();

  // 验证生成的代码, 检查一致性
  llvm::verifyFunction(*func);
}

llvm::AllocaInst* CodeGen::CreateEntryBlockAlloca(llvm::Function* parent,
                                                  llvm::Type* type,
                                                  std::int32_t align) {
  llvm::IRBuilder<> temp{&parent->getEntryBlock(),
                         parent->getEntryBlock().begin()};
  auto ptr{temp.CreateAlloca(type, nullptr)};
  ptr->setAlignment(align);
  ++alloc_count_;

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
    return Builder.CreateInBoundsGEP(lhs, {rhs});
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

    lhs = CastTo(lhs, Builder.getInt64Ty(), true);
    rhs = CastTo(rhs, Builder.getInt64Ty(), true);

    auto value{SubOp(lhs, rhs, true)};
    return DivOp(value, Builder.getInt64(DataLayout.getTypeAllocSize(type)),
                 true);
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
  } else if (IsFloatingPointTy(value) && to->isIntegerTy()) {
    if (is_unsigned) {
      return Builder.CreateFPToUI(value, to);
    } else {
      return Builder.CreateFPToSI(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isFloatingPointTy()) {
    if (FloatPointRank(value->getType()) > FloatPointRank(to)) {
      return Builder.CreateFPTrunc(value, to);
    } else {
      return Builder.CreateFPExt(value, to);
    }
  } else if (IsPointerTy(value) && to->isIntegerTy()) {
    return Builder.CreatePtrToInt(value, to);
  } else if (IsIntegerTy(value) && to->isPointerTy()) {
    return Builder.CreateIntToPtr(value, to);
  } else if (to->isVoidTy() || value->getType() == to) {
    // TODO 这里直接 == 可以吗
    return value;
  } else if (IsArrCastToPtr(value, to)) {
    return llvm::GetElementPtrInst::CreateInBounds(
        value, {Builder.getInt64(0), Builder.getInt64(0)}, "",
        Builder.GetInsertBlock());
  } else if (IsPointerTy(value) && to->isPointerTy()) {
    return Builder.CreatePointerCast(value, to);
  } else {
    Error("{} to {}", LLVMTypeToStr(value->getType()), LLVMTypeToStr(to));
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
  llvm::Value* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = Builder.CreateICmpULE(lhs, rhs);
    } else {
      value = Builder.CreateICmpSLE(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOLE(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = Builder.CreateICmpULE(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return CastTo(value, Builder.getInt32Ty(), true);
}

llvm::Value* CodeGen::LessOp(llvm::Value* lhs, llvm::Value* rhs,
                             bool is_unsigned) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = Builder.CreateICmpULT(lhs, rhs);
    } else {
      value = Builder.CreateICmpSLT(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOLT(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = Builder.CreateICmpULT(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return CastTo(value, Builder.getInt32Ty(), true);
}

llvm::Value* CodeGen::GreaterEqualOp(llvm::Value* lhs, llvm::Value* rhs,
                                     bool is_unsigned) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = Builder.CreateICmpUGE(lhs, rhs);
    } else {
      value = Builder.CreateICmpSGE(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOGE(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = Builder.CreateICmpUGE(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return CastTo(value, Builder.getInt32Ty(), true);
}

llvm::Value* CodeGen::GreaterOp(llvm::Value* lhs, llvm::Value* rhs,
                                bool is_unsigned) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = Builder.CreateICmpUGT(lhs, rhs);
    } else {
      value = Builder.CreateICmpSGT(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOGT(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = Builder.CreateICmpUGT(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return CastTo(value, Builder.getInt32Ty(), true);
}

llvm::Value* CodeGen::EqualOp(llvm::Value* lhs, llvm::Value* rhs) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    value = Builder.CreateICmpEQ(lhs, rhs);
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOEQ(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return CastTo(value, Builder.getInt32Ty(), true);
}

llvm::Value* CodeGen::NotEqualOp(llvm::Value* lhs, llvm::Value* rhs) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    value = Builder.CreateICmpNE(lhs, rhs);
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpONE(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return CastTo(value, Builder.getInt32Ty(), true);
}

llvm::Value* CodeGen::CastToBool(llvm::Value* value) {
  if (value->getType()->isIntegerTy(1)) {
    return value;
  }

  if (IsIntegerTy(value) || IsPointerTy(value)) {
    return Builder.CreateICmpNE(value, GetZero(value->getType()));
  } else if (IsFloatingPointTy(value)) {
    return Builder.CreateFCmpONE(value, GetZero(value->getType()));
  } else {
    Error("{}", LLVMTypeToStr(value->getType()));
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
    return llvm::ConstantInt::get(type, 0);
  } else if (type->isFloatingPointTy()) {
    return llvm::ConstantFP::get(type, 0.0);
  } else if (type->isPointerTy()) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
  } else {
    assert(false);
    return nullptr;
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
    return 0;
  }
}

llvm::Value* CodeGen::LogicOrOp(const BinaryOpExpr& node) {
  auto block{Builder.GetInsertBlock()};
  auto parent_func{block->getParent()};
  auto rhs_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  node.lhs_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, after_block, rhs_block);

  Builder.SetInsertPoint(rhs_block);
  node.rhs_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateBr(after_block);

  Builder.SetInsertPoint(after_block);
  auto phi{Builder.CreatePHI(Builder.getInt1Ty(), 2)};
  phi->addIncoming(Builder.getTrue(), block);
  phi->addIncoming(result_, rhs_block);

  return CastTo(phi, Builder.getInt32Ty(), true);
}

llvm::Value* CodeGen::LogicAndOp(const BinaryOpExpr& node) {
  auto block{Builder.GetInsertBlock()};
  auto parent_func{block->getParent()};
  auto rhs_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  node.lhs_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, rhs_block, after_block);

  Builder.SetInsertPoint(rhs_block);
  node.rhs_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateBr(after_block);

  Builder.SetInsertPoint(after_block);
  auto phi{Builder.CreatePHI(Builder.getInt1Ty(), 2)};
  phi->addIncoming(Builder.getFalse(), block);
  phi->addIncoming(result_, rhs_block);

  return CastTo(phi, Builder.getInt32Ty(), true);
}

llvm::Value* CodeGen::AssignOp(const BinaryOpExpr& node) {
  auto backup{load_};
  load_ = true;
  node.rhs_->Accept(*this);
  load_ = backup;
  auto rhs{result_};
  auto ptr{GetPtr(*node.lhs_)};
  Assign(ptr, rhs, align_);
  return Builder.CreateAlignedLoad(ptr, load_);
}

// * / .(maybe) / obj
llvm::Value* CodeGen::GetPtr(const AstNode& node) {
  if (node.Kind() == AstNodeType::kObjectExpr) {
    auto obj{dynamic_cast<const ObjectExpr&>(node)};
    align_ = obj.GetAlign();
    if (obj.InGlobal() || obj.IsStatic()) {
      return obj.GetGlobalPtr();
    } else {
      return obj.GetLocalPtr();
    }
  } else if (node.Kind() == AstNodeType::kIdentifierExpr) {
    // 函数指针
    node.Accept(*this);
    return result_;
  } else if (node.Kind() == AstNodeType::kUnaryOpExpr) {
    auto unary{dynamic_cast<const UnaryOpExpr&>(node)};
    assert(unary.op_ == Tag::kStar);
    unary.expr_->Accept(*this);
    return result_;
  } else if (node.Kind() == AstNodeType::kBinaryOpExpr) {
    auto binary{dynamic_cast<const BinaryOpExpr&>(node)};
    if (binary.op_ == Tag::kPeriod) {
      auto lhs_ptr{GetPtr(*binary.lhs_)};
      auto obj{dynamic_cast<ObjectExpr*>(binary.rhs_)};
      assert(obj != nullptr);

      auto indexs{obj->GetIndexs()};
      for (const auto& [type, index] : indexs) {
        if (type->IsStructTy()) {
          lhs_ptr = Builder.CreateStructGEP(lhs_ptr, index);
        } else {
          lhs_ptr = Builder.CreateBitCast(
              lhs_ptr,
              type->StructGetMemberType(index)->GetLLVMType()->getPointerTo());
        }
      }
      return lhs_ptr;
    }
  }

  assert(false);
  return nullptr;
}

llvm::Value* CodeGen::Assign(llvm::Value* lhs_ptr, llvm::Value* rhs,
                             std::int32_t align) {
  return Builder.CreateAlignedStore(rhs, lhs_ptr, align);
}

void CodeGen::VisitForNoInc(const ForStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto cond_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(cond_block);
  Builder.SetInsertPoint(cond_block);

  node.cond_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, loop_block, after_block);

  Builder.SetInsertPoint(loop_block);

  PushBlock(cond_block, after_block);
  node.block_->Accept(*this);

  if (!HasBrOrReturn()) {
    Builder.CreateBr(cond_block);
  }
  PopBlock();

  Builder.SetInsertPoint(after_block);
}

void CodeGen::VisitForNoCond(const ForStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto inc_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(loop_block);
  Builder.SetInsertPoint(loop_block);

  Builder.SetInsertPoint(loop_block);

  PushBlock(inc_block, after_block);
  node.block_->Accept(*this);

  Builder.CreateBr(inc_block);

  Builder.SetInsertPoint(inc_block);

  node.inc_->Accept(*this);
  if (!HasBrOrReturn()) {
    Builder.CreateBr(loop_block);
  }
  PopBlock();

  Builder.SetInsertPoint(after_block);
}

void CodeGen::VisitForNoIncCond(const ForStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(loop_block);

  Builder.SetInsertPoint(loop_block);

  PushBlock(loop_block, after_block);
  node.block_->Accept(*this);

  if (!HasBrOrReturn()) {
    Builder.CreateBr(loop_block);
  }
  PopBlock();

  Builder.SetInsertPoint(after_block);
}

void CodeGen::PushBlock(llvm::BasicBlock* continue_block,
                        llvm::BasicBlock* break_stack) {
  continue_stack_.push(continue_block);
  break_stack_.push(break_stack);
  has_br_or_return_.push({false, false});
}

void CodeGen::PopBlock() {
  continue_stack_.pop();
  break_stack_.pop();
  has_br_or_return_.pop();
}

bool CodeGen::HasBrOrReturn() const {
  auto [br, ret]{has_br_or_return_.top()};
  return br || ret;
}

llvm::Value* CodeGen::NegOp(llvm::Value* value, bool is_unsigned) {
  if (IsIntegerTy(value)) {
    if (is_unsigned) {
      return Builder.CreateNeg(value);
    } else {
      return Builder.CreateNSWNeg(value);
    }
  } else if (IsFloatingPointTy(value)) {
    return Builder.CreateFNeg(value);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::LogicNotOp(llvm::Value* value) {
  value = CastToBool(value);
  value = Builder.CreateXor(value, Builder.getTrue());

  return CastTo(value, Builder.getInt32Ty(), true);
}

std::string CodeGen::LLVMTypeToStr(llvm::Type* type) const {
  std::string s;
  llvm::raw_string_ostream rso{s};
  type->print(rso);
  return rso.str();
}

llvm::Value* CodeGen::IncOrDec(const Expr& expr, bool is_inc, bool is_postfix) {
  auto is_unsigned{expr.GetType()->IsUnsigned()};
  auto lhs_ptr{GetPtr(expr)};

  auto lhs_value{Builder.CreateAlignedLoad(lhs_ptr, align_)};
  llvm::Value* rhs_value{};

  llvm::Value* one_value{};
  auto type{expr.GetType()->GetLLVMType()};

  if (type->isIntegerTy()) {
    one_value = llvm::ConstantInt::get(type, 1);
  } else if (type->isFloatingPointTy()) {
    one_value = llvm::ConstantFP::get(type, 1.0);
  } else if (type->isPointerTy()) {
    one_value = Builder.getInt32(1);
  } else {
    assert(false);
  }

  if (is_inc) {
    rhs_value = AddOp(lhs_value, one_value, is_unsigned);
  } else {
    rhs_value = AddOp(lhs_value, NegOp(one_value, false), is_unsigned);
  }

  Assign(lhs_ptr, rhs_value, align_);

  return is_postfix ? lhs_value : rhs_value;
}

bool CodeGen::HasReturn() const { return has_br_or_return_.top().second; }

void CodeGen::EnterFunc() { PushBlock(nullptr, nullptr); }

void CodeGen::ExitFunc() { PopBlock(); }

bool CodeGen::IsArrCastToPtr(llvm::Value* value, llvm::Type* type) {
  auto value_type{value->getType()};

  return value_type->isPointerTy() &&
         value_type->getPointerElementType()->isArrayTy() &&
         type->isPointerTy() &&
         value_type->getPointerElementType()
                 ->getArrayElementType()
                 ->getPointerTo() == type;
}

void CodeGen::InitLocalAggregate(const Declaration& node) {
  auto obj{node.GetObject()};
  auto width{obj->GetType()->GetWidth()};

  Builder.CreateMemSet(
      Builder.CreateBitCast(obj->GetLocalPtr(), Builder.getInt8PtrTy()),
      Builder.getInt8(0), width, obj->GetAlign());

  if (node.value_init_) {
    return;
  }

  for (const auto& item : node.GetLocalInits()) {
    auto backup{load_};
    load_ = true;
    item.GetExpr()->Accept(*this);
    load_ = backup;
    auto value{result_};

    llvm::Value* ptr{obj->GetLocalPtr()};
    for (const auto& [type, index] : item.GetIndexs()) {
      if (type->IsArrayTy()) {
        ptr = Builder.CreateInBoundsGEP(
            ptr, {Builder.getInt64(0), Builder.getInt64(index)});
      } else if (type->IsStructTy()) {
        ptr = Builder.CreateStructGEP(ptr, index);
      } else if (type->IsUnionTy()) {
        ptr = Builder.CreateBitCast(
            ptr,
            type->StructGetMemberType(index)->GetLLVMType()->getPointerTo());
      } else {
        break;
      }
    }

    result_ =
        Builder.CreateAlignedStore(value, ptr, item.GetType()->GetAlign());
  }
}

void CodeGen::DealGlobalDecl(const Declaration& node) {
  auto obj{node.GetObject()};
  auto type{obj->GetType()->GetLLVMType()};
  auto name{obj->GetName()};

  llvm::GlobalVariable* ptr;
  if (obj->HasGlobalPtr()) {
    ptr = obj->GetGlobalPtr();
  } else {
    // 在符号表中查找指定的全局变量, 如果不存在则添加并返回它
    Module->getOrInsertGlobal(name, type);
    ptr = Module->getNamedGlobal(name);
    obj->SetGlobalPtr(ptr);
  }

  ptr->setAlignment(obj->GetAlign());
  if (obj->IsStatic()) {
    ptr->setLinkage(llvm::GlobalVariable::InternalLinkage);
  } else if (obj->IsExtern()) {
    ptr->setLinkage(llvm::GlobalVariable::ExternalLinkage);
  } else {
    ptr->setDSOLocal(true);

    if (!node.HasGlobalInit()) {
      // ptr->setLinkage(llvm::GlobalVariable::CommonLinkage);
    }
  }

  if (node.HasGlobalInit()) {
    ptr->setInitializer(node.GetGlobalInit());
  } else {
    if (!obj->IsExtern()) {
      ptr->setInitializer(GetConstantZero(type));
    }
  }
}

void CodeGen::DealLocaleDecl(const Declaration& node) {
  auto obj{node.GetObject()};
  auto type{obj->GetType()};
  auto name{obj->GetName()};

  if (obj->IsStatic()) {
    llvm::GlobalVariable* ptr;
    auto static_name{obj->GetStaticName() + "." + obj->GetName()};

    if (obj->HasGlobalPtr()) {
      ptr = obj->GetGlobalPtr();
    } else {
      Module->getOrInsertGlobal(static_name, type->GetLLVMType());
      ptr = Module->getNamedGlobal(static_name);
      obj->SetGlobalPtr(ptr);
    }

    ptr->setAlignment(obj->GetAlign());
    ptr->setLinkage(llvm::GlobalVariable::InternalLinkage);

    if (node.HasGlobalInit()) {
      ptr->setInitializer(node.GetGlobalInit());
    } else {
      if (!obj->IsExtern()) {
        ptr->setInitializer(GetConstantZero(type->GetLLVMType()));
      }
    }

    return;
  }

  obj->SetLocalPtr(CreateEntryBlockAlloca(Builder.GetInsertBlock()->getParent(),
                                          type->GetLLVMType(),
                                          obj->GetAlign()));

  if (node.HasLocalInit()) {
    if (type->IsScalarTy()) {
      auto init{node.GetLocalInits()};
      assert(std::size(init) == 1);

      init.front().GetExpr()->Accept(*this);
      Builder.CreateAlignedStore(result_, obj->GetLocalPtr(), obj->GetAlign());
    } else if (type->IsAggregateTy()) {
      InitLocalAggregate(node);
    } else {
      assert(false);
    }
  }
}

bool CodeGen::MayCallBuiltinFunc(const FuncCallExpr& node) {
  auto func_name{node.GetFuncType()->FuncGetName()};

  if (func_name == "__builtin_va_start") {
    auto func_type{llvm::FunctionType::get(Builder.getVoidTy(),
                                           {Builder.getInt8PtrTy()}, false)};
    if (!va_start_) {
      va_start_ =
          llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                                 "llvm.va_start", Module.get());
    }

    assert(std::size(node.args_) == 2);

    node.args_.front()->Accept(*this);

    result_ = Builder.CreateBitCast(result_, Builder.getInt8PtrTy());
    result_ = Builder.CreateCall(va_start_, {result_});

    return true;
  } else if (func_name == "__builtin_va_end") {
    auto func_type{llvm::FunctionType::get(Builder.getVoidTy(),
                                           {Builder.getInt8PtrTy()}, false)};
    if (!va_end_) {
      va_end_ =
          llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                                 "llvm.va_end", Module.get());
    }

    assert(std::size(node.args_) == 1);

    node.args_.front()->Accept(*this);

    result_ = Builder.CreateBitCast(result_, Builder.getInt8PtrTy());
    result_ = Builder.CreateCall(va_end_, {result_});

    return true;
  } else if (func_name == "__builtin_va_arg_sub") {
    assert(std::size(node.args_) == 1);

    node.args_.front()->Accept(*this);
    assert(node.va_arg_type_ != nullptr);

    result_ = VaArg(result_, node.va_arg_type_->GetLLVMType());

    return true;
  } else if (func_name == "__builtin_va_copy") {
    auto func_type{llvm::FunctionType::get(
        Builder.getVoidTy(), {Builder.getInt8PtrTy(), Builder.getInt8PtrTy()},
        false)};
    if (!va_copy_) {
      va_copy_ =
          llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                                 "llvm.va_copy", Module.get());
    }

    assert(std::size(node.args_) == 2);

    node.args_.front()->Accept(*this);
    auto arg1{result_};
    node.args_[1]->Accept(*this);
    auto arg2{result_};

    auto r1{Builder.CreateBitCast(arg1, Builder.getInt8PtrTy())};
    auto r2{Builder.CreateBitCast(arg2, Builder.getInt8PtrTy())};

    result_ = Builder.CreateCall(va_copy_, {r1, r2});

    return true;
  } else {
    return false;
  }
}

llvm::Value* CodeGen::VaArg(llvm::Value* ptr, llvm::Type* type) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto lhs_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto rhs_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  llvm::Value* p{};
  llvm::Value* num{};

  if (type->isIntegerTy() || type->isPointerTy()) {
    p = Builder.CreateStructGEP(ptr, 0);
    num = Builder.CreateAlignedLoad(p, 16);
    result_ = Builder.CreateICmpULE(
        num, llvm::ConstantInt::get(Builder.getInt32Ty(), 40));
  } else if (type->isFloatingPointTy()) {
    p = Builder.CreateStructGEP(ptr, 1);
    num = Builder.CreateAlignedLoad(p, 16);
    result_ = Builder.CreateICmpULE(
        num, llvm::ConstantInt::get(Builder.getInt32Ty(), 160));
  } else {
    assert(false);
  }

  Builder.CreateCondBr(result_, lhs_block, rhs_block);

  Builder.SetInsertPoint(lhs_block);
  result_ = Builder.CreateStructGEP(ptr, 3);
  result_ = Builder.CreateAlignedLoad(result_, 16);
  result_ = Builder.CreateGEP(result_, num);
  auto r{Builder.CreateBitCast(result_, type->getPointerTo())};
  result_ =
      Builder.CreateAdd(num, llvm::ConstantInt::get(Builder.getInt32Ty(), 8));
  Builder.CreateAlignedStore(result_, p, 16);
  Builder.CreateBr(after_block);

  Builder.SetInsertPoint(rhs_block);
  auto pp{Builder.CreateStructGEP(ptr, 2)};
  result_ = Builder.CreateAlignedLoad(pp, 8);
  auto rr{Builder.CreateBitCast(result_, type->getPointerTo())};
  result_ = Builder.CreateGEP(result_,
                              llvm::ConstantInt::get(Builder.getInt32Ty(), 8));
  Builder.CreateAlignedStore(result_, pp, 8);
  Builder.CreateBr(after_block);

  Builder.SetInsertPoint(after_block);
  auto phi{Builder.CreatePHI(type->getPointerTo(), 2)};
  phi->addIncoming(r, lhs_block);
  phi->addIncoming(rr, rhs_block);

  return phi;
}

}  // namespace kcc
