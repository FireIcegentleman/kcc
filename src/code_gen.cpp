//
// Created by kaiser on 2019/11/2.
//

#include "code_gen.h"

#include <cassert>
#include <memory>
#include <vector>

#include <llvm/ADT/Optional.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include "calc.h"
#include "error.h"
#include "llvm_common.h"
#include "util.h"

namespace kcc {

CodeGen::CodeGen() {
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

  // 配置模块以指定目标机器和数据布局
  Module->setTargetTriple(target_triple);
  Module->setDataLayout(TargetMachine->createDataLayout());
}

void CodeGen::GenCode(const TranslationUnit* root) {
  root->Accept(*this);

  if (llvm::verifyModule(*Module, &llvm::errs())) {
#ifdef DEV
    Warning("module is broken");
#else
    Error("module is broken");
#endif
  }
}

CodeGen::BreakContinue::BreakContinue(llvm::BasicBlock* break_block,
                                      llvm::BasicBlock* continue_block)
    : break_block(break_block), continue_block(continue_block) {}

llvm::BasicBlock* CodeGen::CreateBasicBlock(const std::string& name,
                                            llvm::Function* parent,
                                            llvm::BasicBlock* insert_before) {
  (void)name;
#ifdef NDEBUG
  return llvm::BasicBlock::Create(Context, "", parent, insert_before);
#else
  return llvm::BasicBlock::Create(Context, name, parent, insert_before);
#endif
}

llvm::AllocaInst* CodeGen::CreateEntryBlockAlloca(llvm::Type* type,
                                                  std::int32_t align) {
  auto ptr{new llvm::AllocaInst{type, 0, "", alloc_insert_point_}};
  ptr->setAlignment(align);

  return ptr;
}

void CodeGen::EmitBranchOnBoolExpr(const Expr* expr,
                                   llvm::BasicBlock* true_block,
                                   llvm::BasicBlock* false_block) {
  if (auto cond_binary{dynamic_cast<const BinaryOpExpr*>(expr)}) {
    if (cond_binary->GetOp() == Tag::kAmpAmp) {
      auto lhs_true_block{CreateBasicBlock("logic.and.lhs.true")};
      EmitBranchOnBoolExpr(cond_binary->GetLHS(), lhs_true_block, false_block);
      EmitBlock(lhs_true_block);
      EmitBranchOnBoolExpr(cond_binary->GetRHS(), true_block, false_block);
      return;
    } else if (cond_binary->GetOp() == Tag::kPipePipe) {
      auto lhs_false_block{CreateBasicBlock("logic.or.lhs.false")};
      EmitBranchOnBoolExpr(cond_binary->GetLHS(), true_block, lhs_false_block);
      EmitBlock(lhs_false_block);
      EmitBranchOnBoolExpr(cond_binary->GetRHS(), true_block, false_block);
      return;
    }
  } else if (auto cond_unary{dynamic_cast<const UnaryOpExpr*>(expr)}) {
    if (cond_unary->GetOp() == Tag::kExclaim) {
      EmitBranchOnBoolExpr(cond_unary->GetExpr(), false_block, true_block);
      return;
    }
  } else if (auto cond_op{dynamic_cast<const ConditionOpExpr*>(expr)}) {
    auto lhs_block{CreateBasicBlock("cond.true")};
    auto rhs_block{CreateBasicBlock("cond.false")};
    EmitBranchOnBoolExpr(cond_op->GetCond(), lhs_block, rhs_block);
    EmitBlock(lhs_block);
    EmitBranchOnBoolExpr(cond_op->GetLHS(), true_block, false_block);
    EmitBlock(rhs_block);
    EmitBranchOnBoolExpr(cond_op->GetRHS(), true_block, false_block);
    return;
  }

  Builder.CreateCondBr(EvaluateExprAsBool(expr), true_block, false_block);
}

void CodeGen::EmitBlock(llvm::BasicBlock* bb, bool is_finished) {
  EmitBranch(bb);

  if (is_finished && bb->use_empty()) {
    delete bb;
    return;
  }

  func_->getBasicBlockList().push_back(bb);
  Builder.SetInsertPoint(bb);
}

void CodeGen::EmitBranch(llvm::BasicBlock* target) {
  auto curr{Builder.GetInsertBlock()};

  if (curr && !curr->getTerminator()) {
    Builder.CreateBr(target);
  }

  Builder.ClearInsertionPoint();
}

void CodeGen::EmitStmt(const Stmt* stmt) {
  assert(stmt != nullptr);

  // 检查是否可以 emit 而不必考虑 insert point
  if (EmitSimpleStmt(stmt)) {
    return;
  }

  // 检查是否在生成不可访问的代码
  if (!HaveInsertPoint()) {
    // 如果语句不包含 label 则可以不生成它,
    // 这样是安全的, 因为 (1) 改代码不可访问
    // (2) 已经处理了声明
    if (!ContainsLabel(stmt)) {
      assert(stmt->Kind() != AstNodeType::kDeclaration);
      return;
    } else {
      EnsureInsertPoint();
    }
  }

  stmt->Accept(*this);
}

bool CodeGen::EmitSimpleStmt(const Stmt* stmt) {
  switch (stmt->Kind()) {
    case AstNodeType::kLabelStmt:
    case AstNodeType::kCaseStmt:
    case AstNodeType::kDefaultStmt:
    case AstNodeType::kCompoundStmt:
    case AstNodeType::kGotoStmt:
    case AstNodeType::kContinueStmt:
    case AstNodeType::kBreakStmt:
    case AstNodeType::kDeclaration:
      stmt->Accept(*this);
      return true;
    default:
      return false;
  }
}

bool CodeGen::HaveInsertPoint() const {
  return Builder.GetInsertBlock() != nullptr;
}

void CodeGen::EnsureInsertPoint() {
  if (!HaveInsertPoint()) {
    EmitBlock(CreateBasicBlock());
  }
}

llvm::Value* CodeGen::EvaluateExprAsBool(const Expr* expr) {
  assert(expr != nullptr);
  expr->Accept(*this);
  return CastToBool(result_);
}

bool CodeGen::TestAndClearIgnoreResultAssign() {
  bool ret{ignore_result_assign_};
  ignore_result_assign_ = false;
  return ret;
}

void CodeGen::PushBlock(llvm::BasicBlock* break_stack,
                        llvm::BasicBlock* continue_block) {
  break_continue_stack_.push({break_stack, continue_block});
}

void CodeGen::PopBlock() { break_continue_stack_.pop(); }

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
    assert(unary.GetOp() == Tag::kStar);
    unary.GetExpr()->Accept(*this);
    return result_;
  } else if (node.Kind() == AstNodeType::kBinaryOpExpr) {
    auto binary{dynamic_cast<const BinaryOpExpr&>(node)};
    if (binary.GetOp() == Tag::kPeriod) {
      auto lhs_ptr{GetPtr(*binary.GetLHS())};
      auto obj{dynamic_cast<const ObjectExpr*>(binary.GetRHS())};
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

void CodeGen::Visit(const UnaryOpExpr& node) {
  auto is_unsigned{node.GetExpr()->GetType()->IsUnsigned()};

  switch (node.GetOp()) {
    case Tag::kPlusPlus:
      result_ = IncOrDec(*node.GetExpr(), true, false);
      break;
    case Tag::kPostfixPlusPlus:
      result_ = IncOrDec(*node.GetExpr(), true, true);
      break;
    case Tag::kMinusMinus:
      result_ = IncOrDec(*node.GetExpr(), false, false);
      break;
    case Tag::kPostfixMinusMinus:
      result_ = IncOrDec(*node.GetExpr(), false, true);
      break;
    case Tag::kPlus:
      TestAndClearIgnoreResultAssign();
      node.GetExpr()->Accept(*this);
      break;
    case Tag::kMinus:
      TestAndClearIgnoreResultAssign();
      node.GetExpr()->Accept(*this);
      result_ = NegOp(result_, is_unsigned);
      break;
    case Tag::kTilde:
      TestAndClearIgnoreResultAssign();
      node.GetExpr()->Accept(*this);
      result_ = Builder.CreateNot(result_);
      break;
    case Tag::kExclaim:
      node.GetExpr()->Accept(*this);
      result_ = LogicNotOp(result_);
      break;
    case Tag::kStar: {
      auto binary{dynamic_cast<const BinaryOpExpr*>(node.GetExpr())};
      if (binary && binary->GetOp() == Tag::kPlus) {
        // e.g. a[1]
        auto backup{load_};
        load_ = false;
        binary->GetLHS()->Accept(*this);
        auto lhs{result_};
        binary->GetRHS()->Accept(*this);
        load_ = backup;
        if (lhs->getType()->isPointerTy() &&
            lhs->getType()->getPointerElementType()->isArrayTy()) {
          result_ =
              Builder.CreateInBoundsGEP(lhs, {result_, Builder.getInt64(0)});
        } else {
          result_ = Builder.CreateInBoundsGEP(lhs, {result_});
          result_ = Builder.CreateAlignedLoad(result_, align_);
        }
      } else if (node.GetExpr()->GetType()->IsPointerTy() &&
                 node.GetExpr()
                     ->GetType()
                     ->PointerGetElementType()
                     ->IsFunctionTy()) {
        node.GetExpr()->Accept(*this);
      } else {
        result_ = Builder.CreateAlignedLoad(GetPtr(node), align_);
      }
    } break;
    case Tag::kAmp:
      result_ = GetPtr(*node.GetExpr());
      break;
    default:
      assert(false);
  }
}

void CodeGen::Visit(const TypeCastExpr& node) {
  node.GetExpr()->Accept(*this);
  result_ = CastTo(result_, node.GetCastToType()->GetLLVMType(),
                   node.GetExpr()->GetType()->IsUnsigned());
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
  switch (node.GetOp()) {
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

  TestAndClearIgnoreResultAssign();
  node.GetLHS()->Accept(*this);
  auto lhs{result_};
  node.GetRHS()->Accept(*this);
  auto rhs{result_};

  bool is_unsigned{node.GetLHS()->GetType()->IsUnsigned()};

  switch (node.GetOp()) {
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

bool CodeGen::IsCheapEnoughToEvaluateUnconditionally(const Expr* expr) {
  return expr->Kind() == AstNodeType::kConstantExpr;
}

void CodeGen::Visit(const ConditionOpExpr& node) {
  TestAndClearIgnoreResultAssign();

  if (auto cond{CalcConstantExpr{}.Calc(node.GetCond())}) {
    auto live{node.GetLHS()}, dead{node.GetRHS()};
    if (cond->isZeroValue()) {
      std::swap(live, dead);
    }

    live->Accept(*this);
    return;
  }

  // x ? 4 : 5
  if (IsCheapEnoughToEvaluateUnconditionally(node.GetLHS()) &&
      IsCheapEnoughToEvaluateUnconditionally(node.GetRHS())) {
    auto cond{EvaluateExprAsBool(node.GetCond())};
    node.GetLHS()->Accept(*this);
    auto lhs{result_};
    node.GetRHS()->Accept(*this);
    auto rhs{result_};
    Builder.CreateSelect(cond, lhs, rhs);
    return;
  }

  auto lhs_block{CreateBasicBlock("cond.true")};
  auto rhs_block{CreateBasicBlock("cond.false")};
  auto end_block{CreateBasicBlock("cond.end")};

  EmitBranchOnBoolExpr(node.GetCond(), lhs_block, rhs_block);

  EmitBlock(lhs_block);
  node.GetLHS()->Accept(*this);
  auto lhs{result_};
  lhs_block = Builder.GetInsertBlock();
  EmitBranch(end_block);

  EmitBlock(rhs_block);
  node.GetRHS()->Accept(*this);
  auto rhs{result_};
  rhs_block = Builder.GetInsertBlock();
  EmitBranch(end_block);

  EmitBlock(end_block);

  if (lhs->getType()->isVoidTy()) {
    return;
  }

  auto phi{Builder.CreatePHI(lhs->getType(), 2)};
  phi->addIncoming(lhs, lhs_block);
  phi->addIncoming(rhs, rhs_block);

  result_ = phi;
}

// LLVM 默认使用本机 C 调用约定
void CodeGen::Visit(const FuncCallExpr& node) {
  if (MayCallBuiltinFunc(node)) {
    return;
  }

  node.GetCallee()->Accept(*this);
  auto callee{result_};

  std::vector<llvm::Value*> values;
  load_ = true;
  for (auto& item : node.GetArgs()) {
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
    result_ = llvm::ConstantInt::get(Context, node.GetIntegerVal());
  } else if (type->isFloatingPointTy()) {
    result_ = llvm::ConstantFP::get(type, node.GetFloatPointVal());
  } else {
    assert(false);
  }
}

// 1 / 2 / 4
// 注意已经添加空字符了
void CodeGen::Visit(const StringLiteralExpr& node) { result_ = node.GetPtr(); }

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
  result_ = llvm::ConstantInt::get(Builder.getInt32Ty(), node.GetVal());
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

void CodeGen::Visit(const StmtExpr& node) { node.GetBlock()->Accept(*this); }

void CodeGen::EmitLabel(const LabelStmt& label_stmt) {
  EmitBlock(GetBasicBlockForLabel(&label_stmt));
}

void CodeGen::Visit(const LabelStmt& node) {
  EmitLabel(node);
  EmitStmt(node.GetStmt());
}

void CodeGen::Visit(const CaseStmt& node) {
  auto block{CreateBasicBlock("switch.case")};
  EmitBlock(block);
  std::int64_t begin, end;

  if (auto rhs{node.GetRHS()}) {
    begin = node.GetLHS();
    end = *rhs;
  } else {
    begin = node.GetLHS();
    end = begin;
  }

  for (auto i{begin}; i <= end; ++i) {
    switch_inst_->addCase(llvm::ConstantInt::get(Builder.getInt64Ty(), i),
                          block);
  }

  EmitStmt(node.GetStmt());
}

void CodeGen::Visit(const DefaultStmt& node) {
  auto default_block{switch_inst_->getDefaultDest()};
  assert(default_block->empty());
  EmitBlock(default_block);
  EmitStmt(node.GetStmt());
}

void CodeGen::Visit(const CompoundStmt& node) {
  for (auto& item : node.GetStmts()) {
    EmitStmt(item);
  }
}

void CodeGen::Visit(const ExprStmt& node) {
  if (node.GetExpr()) {
    node.GetExpr()->Accept(*this);
  } else {
    return;
  }

  if (auto bb{Builder.GetInsertBlock()}) {
    if (bb->empty() && bb->use_empty()) {
      bb->eraseFromParent();
      Builder.ClearInsertionPoint();
    }
  }
}

bool CodeGen::ContainsLabel(const Stmt* stmt, bool ignore_case) {
  if (stmt == nullptr) {
    return false;
  }

  if (stmt->Kind() == AstNodeType::kLabelStmt ||
      stmt->Kind() == AstNodeType::kCaseStmt ||
      stmt->Kind() == AstNodeType::kDefaultStmt) {
    return true;
  }

  if (stmt->Kind() == AstNodeType::kSwitchStmt && !ignore_case) {
    return true;
  }

  if (stmt->Kind() == AstNodeType::kSwitchStmt) {
    ignore_case = true;
  }

  for (const auto& item : stmt->Children()) {
    if (ContainsLabel(item, ignore_case)) {
      return true;
    }
  }

  return false;
}

void CodeGen::Visit(const IfStmt& node) {
  if (auto cond{CalcConstantExpr{}.Calc(node.GetCond())}) {
    auto executed{node.GetThen()}, skipped{node.GetElse()};
    if (cond->isZeroValue()) {
      std::swap(executed, skipped);
    }

    // 如果 skipped block 不包含 label, 那么可以不生成它
    if (!ContainsLabel(skipped)) {
      if (executed) {
        EmitStmt(executed);
      }
      return;
    }
  }

  auto then_block{CreateBasicBlock("if.then")};
  auto end_block{CreateBasicBlock("if.end")};
  auto else_block{end_block};

  if (node.GetElse()) {
    else_block = CreateBasicBlock("if.else");
  }

  EmitBranchOnBoolExpr(node.GetCond(), then_block, else_block);

  EmitBlock(then_block);
  EmitStmt(node.GetThen());
  EmitBranch(end_block);

  if (auto else_stmt{node.GetElse()}) {
    EmitBlock(else_block);
    EmitStmt(else_stmt);
    EmitBranch(end_block);
  }

  EmitBlock(end_block, true);
}

void CodeGen::Visit(const SwitchStmt& node) {
  node.GetCond()->Accept(*this);
  auto cond_val{result_};

  auto switch_inst_backup{switch_inst_};

  auto default_block{CreateBasicBlock("switch.default")};
  auto end_block{CreateBasicBlock("switch.end")};
  switch_inst_ = Builder.CreateSwitch(cond_val, default_block);

  Builder.ClearInsertionPoint();

  llvm::BasicBlock* continue_block{};
  if (!std::empty(break_continue_stack_)) {
    continue_block = break_continue_stack_.top().continue_block;
  }
  PushBlock(end_block, continue_block);
  EmitStmt(node.GetStmt());
  PopBlock();

  if (!default_block->getParent()) {
    default_block->replaceAllUsesWith(end_block);
    delete default_block;
  }

  EmitBlock(end_block, true);

  switch_inst_ = switch_inst_backup;
}
void CodeGen::SimplifyForwardingBlocks(llvm::BasicBlock* bb) {
  auto bi{llvm::dyn_cast<llvm::BranchInst>(bb->getTerminator())};

  if (!bi || !bi->isUnconditional()) {
    return;
  }

  // getSuccessor 获取要跳转到的 BasicBlock
  bb->replaceAllUsesWith(bi->getSuccessor(0));
  bi->eraseFromParent();
  bb->eraseFromParent();
}
void CodeGen::Visit(const WhileStmt& node) {
  auto cond_block{CreateBasicBlock("while.cond")};
  EmitBlock(cond_block);

  auto body_block{CreateBasicBlock("while.body")};
  auto end_block{CreateBasicBlock("while.end")};

  PushBlock(end_block, cond_block);

  auto cond_val{EvaluateExprAsBool(node.GetCond())};

  // while(1)
  bool emit_br{true};
  if (auto constant{llvm::dyn_cast<llvm::ConstantInt>(cond_val)}) {
    if (constant->isOne()) {
      emit_br = false;
    }
  }
  if (emit_br) {
    Builder.CreateCondBr(cond_val, body_block, end_block);
  }

  EmitBlock(body_block);
  EmitStmt(node.GetBlock());
  PopBlock();

  EmitBranch(cond_block);
  EmitBlock(end_block, true);

  if (!emit_br) {
    SimplifyForwardingBlocks(cond_block);
  }
}

void CodeGen::Visit(const DoWhileStmt& node) {
  auto body_block{CreateBasicBlock("do.while.body")};
  auto cond_block{CreateBasicBlock("do.while.cond")};
  auto end_block{CreateBasicBlock("do.while.end")};

  EmitBlock(body_block);
  PushBlock(end_block, cond_block);
  EmitStmt(node.GetBlock());
  PopBlock();

  EmitBlock(cond_block);
  auto cond_val{EvaluateExprAsBool(node.GetCond())};

  // do{}while(0)
  bool emit_br{true};
  if (auto constant{llvm::dyn_cast<llvm::ConstantInt>(cond_val)}) {
    if (constant->isZero()) {
      emit_br = false;
    }
  }
  if (emit_br) {
    Builder.CreateCondBr(cond_val, body_block, end_block);
  }

  EmitBlock(end_block);

  if (!emit_br) {
    SimplifyForwardingBlocks(cond_block);
  }
}

void CodeGen::Visit(const ForStmt& node) {
  if (node.GetInit()) {
    node.GetInit()->Accept(*this);
  } else if (node.GetDecl()) {
    EmitStmt(node.GetDecl());
  }

  auto cond_block{CreateBasicBlock("for.cond")};
  auto end_block{CreateBasicBlock("for.end")};

  EmitBlock(cond_block);

  if (node.GetCond()) {
    auto body_block{CreateBasicBlock("for.body")};
    EmitBranchOnBoolExpr(node.GetCond(), body_block, end_block);
    EmitBlock(body_block);
  }

  llvm::BasicBlock* continue_block;
  if (node.GetInc()) {
    continue_block = CreateBasicBlock("for.inc");
  } else {
    continue_block = cond_block;
  }

  PushBlock(end_block, continue_block);
  EmitStmt(node.GetBlock());
  PopBlock();

  if (node.GetInc()) {
    EmitBlock(continue_block);
    node.GetInc()->Accept(*this);
  }

  EmitBranch(cond_block);

  EmitBlock(end_block, true);
}

void CodeGen::Visit(const GotoStmt& node) {
  EmitBranchThroughCleanup(GetBasicBlockForLabel(node.GetLabel()));
}

void CodeGen::Visit(const ContinueStmt& node) {
  if (std::empty(break_continue_stack_)) {
    Error(node.GetLoc(), "continue stmt not in a loop or switch");
  } else {
    EmitBranchThroughCleanup(break_continue_stack_.top().continue_block);
  }
}

void CodeGen::Visit(const BreakStmt& node) {
  if (std::empty(break_continue_stack_)) {
    Error(node.GetLoc(), "break stmt not in a loop or switch");
  } else {
    EmitBranchThroughCleanup(break_continue_stack_.top().break_block);
  }
}

void CodeGen::EmitBranchThroughCleanup(llvm::BasicBlock* dest) {
  if (!HaveInsertPoint()) {
    return;
  }

  Builder.CreateBr(dest);
  Builder.ClearInsertionPoint();
}

void CodeGen::Visit(const ReturnStmt& node) {
  auto expr{node.GetExpr()};

  if (!expr) {
    EmitBranchThroughCleanup(return_block_);
    return;
  }

  if (return_value_) {
    auto back{load_};
    load_ = true;
    node.GetExpr()->Accept(*this);
    load_ = back;
    Builder.CreateStore(result_, return_value_);
  } else {
    Error(node.GetLoc(), "void function '{}' should not return a value",
          func_->getName().str());
  }

  EmitBranchThroughCleanup(return_block_);
}

void CodeGen::Visit(const TranslationUnit& node) {
  for (const auto& item : node.GetExtDecl()) {
    item->Accept(*this);
  }
}

void CodeGen::Visit(const Declaration& node) {
  // 对于函数声明, 当函数调用或者定义时处理
  // 对于全局变量和局部静态变量, 在语法分析时已经处理完了
  if (!node.IsObjDecl() || node.IsObjDeclInGlobal()) {
    return;
  }

  DealLocaleDecl(node);
}

void CodeGen::Visit(const FuncDef& node) {
  StartFunction(node);
  EmitStmt(node.GetBody());
  FinishFunction(node);
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
    return DivOp(
        value, Builder.getInt64(Module->getDataLayout().getTypeAllocSize(type)),
        true);
  } else {
    assert(false);
    return nullptr;
  }
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

llvm::Value* CodeGen::LogicOrOp(const BinaryOpExpr& node) {
  if (auto cond{CalcConstantExpr{}.Calc(node.GetLHS())}) {
    if (cond->isZeroValue()) {
      auto rhs{EvaluateExprAsBool(node.GetRHS())};
      return Builder.CreateZExt(rhs, Builder.getInt32Ty());
    } else {
      return Builder.getInt32(1);
    }
  }

  auto end_block{CreateBasicBlock("logic.or.end")};
  auto rhs_block{CreateBasicBlock("logic.or.rhs")};

  EmitBranchOnBoolExpr(node.GetLHS(), end_block, rhs_block);
  auto phi{llvm::PHINode::Create(Builder.getInt1Ty(), 2, "", end_block)};

  for (auto begin{llvm::pred_begin(end_block)};
       begin != llvm::pred_end(end_block); ++begin) {
    phi->addIncoming(Builder.getTrue(), *begin);
  }

  EmitBlock(rhs_block);
  auto rhs_value{EvaluateExprAsBool(node.GetRHS())};

  rhs_block = Builder.GetInsertBlock();
  EmitBlock(end_block);

  phi->addIncoming(rhs_value, rhs_block);

  return Builder.CreateZExt(phi, Builder.getInt32Ty());
}

llvm::Value* CodeGen::LogicAndOp(const BinaryOpExpr& node) {
  if (auto cond{CalcConstantExpr{}.Calc(node.GetLHS())}) {
    if (cond->isOneValue()) {
      auto rhs{EvaluateExprAsBool(node.GetRHS())};
      return Builder.CreateZExt(rhs, Builder.getInt32Ty());
    } else {
      return Builder.getInt32(0);
    }
  }

  auto end_block{CreateBasicBlock("logic.and.end")};
  auto rhs_block{CreateBasicBlock("logic.and.rhs")};

  EmitBranchOnBoolExpr(node.GetLHS(), rhs_block, end_block);
  auto phi{llvm::PHINode::Create(Builder.getInt1Ty(), 2, "", end_block)};

  for (auto begin{llvm::pred_begin(end_block)};
       begin != llvm::pred_end(end_block); ++begin) {
    phi->addIncoming(Builder.getFalse(), *begin);
  }

  EmitBlock(rhs_block);
  auto rhs_value{EvaluateExprAsBool(node.GetRHS())};

  rhs_block = Builder.GetInsertBlock();
  EmitBlock(end_block);

  phi->addIncoming(rhs_value, rhs_block);

  return Builder.CreateZExt(phi, Builder.getInt32Ty());
}

llvm::Value* CodeGen::AssignOp(const BinaryOpExpr& node) {
  auto ignore{TestAndClearIgnoreResultAssign()};

  auto backup{load_};
  load_ = true;
  node.GetRHS()->Accept(*this);
  load_ = backup;
  auto rhs{result_};
  auto ptr{GetPtr(*node.GetLHS())};
  Assign(ptr, rhs, align_);

  if (!ignore) {
    return Builder.CreateAlignedLoad(ptr, load_);
  } else {
    return nullptr;
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

    assert(std::size(node.GetArgs()) == 2);

    node.GetArgs().front()->Accept(*this);

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

    assert(std::size(node.GetArgs()) == 1);

    node.GetArgs().front()->Accept(*this);

    result_ = Builder.CreateBitCast(result_, Builder.getInt8PtrTy());
    result_ = Builder.CreateCall(va_end_, {result_});

    return true;
  } else if (func_name == "__builtin_va_arg_sub") {
    assert(std::size(node.GetArgs()) == 1);

    node.GetArgs().front()->Accept(*this);
    assert(node.GetVaArgType() != nullptr);

    result_ = VaArg(result_, node.GetVaArgType()->GetLLVMType());

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

    assert(std::size(node.GetArgs()) == 2);

    node.GetArgs().front()->Accept(*this);
    auto arg1{result_};
    node.GetArgs()[1]->Accept(*this);
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

  if (type->isIntegerTy() || type->isPointerTy()) {
    result_ =
        Builder.CreateAdd(num, llvm::ConstantInt::get(Builder.getInt32Ty(), 8));
  } else if (type->isFloatingPointTy()) {
    result_ = Builder.CreateAdd(
        num, llvm::ConstantInt::get(Builder.getInt32Ty(), 16));
  } else {
    assert(false);
  }

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

llvm::BasicBlock* CodeGen::GetBasicBlockForLabel(const LabelStmt* label) {
  auto& bb{labels_[label]};
  if (bb) {
    return bb;
  }

  return bb = CreateBasicBlock(label->GetName());
}

void CodeGen::DealLocaleDecl(const Declaration& node) {
  auto obj{node.GetObject()};
  auto type{obj->GetType()};
  auto name{obj->GetName()};

  obj->SetLocalPtr(
      CreateEntryBlockAlloca(type->GetLLVMType(), obj->GetAlign()));

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
  } else if (node.HasConstantInit()) {
    auto ptr{obj->GetLocalPtr()};
    if (ptr->getType() != Builder.getInt8PtrTy()) {
      result_ = Builder.CreateBitCast(ptr, Builder.getInt8PtrTy());
    }

    Builder.CreateMemCpy(result_, obj->GetAlign(), node.GetConstant(),
                         obj->GetAlign(), obj->GetType()->GetWidth());
  }
}

void CodeGen::InitLocalAggregate(const Declaration& node) {
  auto obj{node.GetObject()};
  auto width{obj->GetType()->GetWidth()};

  Builder.CreateMemSet(
      Builder.CreateBitCast(obj->GetLocalPtr(), Builder.getInt8PtrTy()),
      Builder.getInt8(0), width, obj->GetAlign());

  if (node.ValueInit()) {
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

void CodeGen::StartFunction(const FuncDef& node) {
  node.GetIdent()->Accept(*this);
  func_ = llvm::cast<llvm::Function>(result_);

  auto func_name{node.GetName()};
  auto func_type{node.GetFuncType()};

  // TODO Attribute DSOLocal 用法
  if (node.GetLinkage() != kInternal) {
    func_->setDSOLocal(true);
  }

  func_->addFnAttr(llvm::Attribute::NoUnwind);
  func_->addFnAttr(llvm::Attribute::StackProtectStrong);
  func_->addFnAttr(llvm::Attribute::UWTable);

  if (OptimizationLevel == OptLevel::kO0) {
    func_->addFnAttr(llvm::Attribute::NoInline);
    func_->addFnAttr(llvm::Attribute::OptimizeNone);
  }

  auto entry{llvm::BasicBlock::Create(Context, "", func_)};

  auto undef{llvm::UndefValue::get(Builder.getInt32Ty())};
  alloc_insert_point_ =
      new llvm::BitCastInst{undef, Builder.getInt32Ty(), "", entry};

  return_block_ = CreateBasicBlock("return");
  return_value_ = nullptr;

  auto return_type{func_type->FuncGetReturnType()};
  if (!return_type->IsVoidTy()) {
    return_value_ = CreateEntryBlockAlloca(return_type->GetLLVMType(),
                                           return_type->GetAlign());
  }

  Builder.SetInsertPoint(entry);

  if (node.GetFuncType()->FuncGetName() == "main") {
    Builder.CreateStore(Builder.getInt32(0), return_value_);
  }

  auto iter{std::begin(func_type->FuncGetParams())};
  for (auto&& arg : func_->args()) {
    auto ptr{CreateEntryBlockAlloca((*iter)->GetType()->GetLLVMType(),
                                    (*iter)->GetType()->GetAlign())};
    (*iter)->SetLocalPtr(ptr);
    // 将参数的值保存到分配的内存中
    Builder.CreateStore(&arg, ptr);
    ++iter;
  }
}

void CodeGen::FinishFunction(const FuncDef& node) {
  auto func_name{node.GetName()};
  auto func{Module->getFunction(func_name)};

  EmitReturnBlock();
  EmitFunctionEpilog();

  auto ptr{alloc_insert_point_};
  alloc_insert_point_ = nullptr;
  ptr->eraseFromParent();

  // 验证生成的代码, 检查一致性
  llvm::verifyFunction(*func);
}

void CodeGen::EmitReturnBlock() {
  auto bb{Builder.GetInsertBlock()};

  if (bb) {
    assert(!bb->getTerminator());

    if (bb->empty() || return_block_->use_empty()) {
      return_block_->replaceAllUsesWith(bb);
      delete return_block_;

      if (return_value_ && func_->getName() != "main") {
        Warning("function '{}' miss return", func_->getName().str());
      }
    } else {
      EmitBlock(return_block_);
    }

    return;
  }

  if (return_block_->hasOneUse()) {
    auto bi{llvm::dyn_cast<llvm::BranchInst>(*return_block_->use_begin())};
    if (bi && bi->isUnconditional() && bi->getSuccessor(0) == return_block_) {
      Builder.SetInsertPoint(bi->getParent());
      bi->eraseFromParent();
      delete return_block_;
      return;
    }
  }

  EmitBlock(return_block_);
}

void CodeGen::EmitFunctionEpilog() {
  llvm::Value* value{};

  if (return_value_) {
    value = Builder.CreateLoad(return_value_);
  }

  if (value) {
    Builder.CreateRet(value);
  } else {
    Builder.CreateRetVoid();
  }
}

}  // namespace kcc
