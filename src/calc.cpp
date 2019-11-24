//
// Created by kaiser on 2019/11/17.
//

#include "calc.h"

#include <cassert>

#include "error.h"
#include "llvm_common.h"

namespace kcc {

llvm::Constant* CalcConstantExpr::Calc(Expr* expr) {
  assert(expr != nullptr);
  expr->Accept(*this);
  assert(val_ != nullptr);
  return val_;
}

std::int64_t CalcConstantExpr::CalcInteger(Expr* expr) {
  auto val{Calc(expr)};
  if (auto p{llvm::dyn_cast<llvm::ConstantInt>(val)}) {
    return p->getValue().getSExtValue();
  } else {
    Error(expr->GetLoc(), "expect integer constant expression, but got '{}",
          LLVMConstantToStr(val));
  }
}

void CalcConstantExpr::Visit(const UnaryOpExpr& node) {
  switch (node.op_) {
    case Tag::kPlus:
      val_ = CalcConstantExpr{}.Calc(node.expr_);
      break;
    case Tag::kMinus:
      val_ = NegOp(CalcConstantExpr{}.Calc(node.expr_),
                   node.GetType()->IsUnsigned());
      break;
    case Tag::kTilde:
      val_ = llvm::ConstantExpr::getNot(CalcConstantExpr{}.Calc(node.expr_));
      break;
    case Tag::kExclaim:
      val_ = LogicNotOp(CalcConstantExpr{}.Calc(node.expr_));
      break;
    case Tag::kAmp: {
      // 运算对象只能是一个全局变量
      auto p{dynamic_cast<ObjectExpr*>(node.expr_)};
      assert(p != nullptr);
      val_ = p->GetGlobalPtr();
    } break;
    default:
      Error(node.GetLoc(), "expect constant expression");
  }
}

void CalcConstantExpr::Visit(const TypeCastExpr& node) {
  val_ = ConstantCastTo(CalcConstantExpr{}.Calc(node.expr_),
                        node.to_->GetLLVMType(),
                        node.expr_->GetType()->IsUnsigned());
}

void CalcConstantExpr::Visit(const BinaryOpExpr& node) {
#define L CalcConstantExpr{}.Calc(node.lhs_)
#define R CalcConstantExpr{}.Calc(node.rhs_)

  auto is_unsigned{node.GetType()->IsUnsigned()};

  switch (node.op_) {
    case Tag::kPlus:
      val_ = AddOp(L, R, is_unsigned);
      break;
    case Tag::kMinus:
      val_ = SubOp(L, R, is_unsigned);
      break;
    case Tag::kStar:
      val_ = MulOp(L, R, is_unsigned);
      break;
    case Tag::kSlash: {
      auto lhs{L};
      auto rhs{R};
      if (rhs->isZeroValue()) {
        Error(node.rhs_, "division by zero");
      }
      val_ = DivOp(lhs, rhs, is_unsigned);
      break;
    }
    case Tag::kPercent: {
      auto lhs{L};
      auto rhs{R};
      if (rhs->isZeroValue()) {
        Error(node.rhs_, "division by zero");
      }
      val_ = ModOp(lhs, rhs, is_unsigned);
      break;
    }
    case Tag::kAmp:
      val_ = AndOp(L, R);
      break;
    case Tag::kPipe:
      val_ = OrOp(L, R);
      break;
    case Tag::kCaret:
      val_ = XorOp(L, R);
      break;
    case Tag::kLessLess:
      val_ = ShlOp(L, R);
      break;
    case Tag::kGreaterGreater:
      val_ = ShrOp(L, R, is_unsigned);
      break;
    case Tag::kAmpAmp:
      val_ = LogicAndOp(node);
      break;
    case Tag::kPipePipe:
      val_ = LogicOrOp(node);
      break;
    case Tag::kEqualEqual:
      val_ = EqualOp(L, R);
      break;
    case Tag::kExclaimEqual:
      val_ = NotEqualOp(L, R);
      break;
    case Tag::kLess:
      val_ = LessOp(L, R, is_unsigned);
      break;
    case Tag::kGreater:
      val_ = GreaterOp(L, R, is_unsigned);
      break;
    case Tag::kLessEqual:
      val_ = LessEqualOp(L, R, is_unsigned);
      break;
    case Tag::kGreaterEqual:
      val_ = GreaterEqualOp(L, R, is_unsigned);
      break;
    default:
      Error(node.GetLoc(), "expect constant expression");
  }

#undef L
#undef R
#undef I_L
#undef I_R
}

void CalcConstantExpr::Visit(const ConditionOpExpr& node) {
  auto flag{CalcConstantExpr{}.Calc(node.cond_)};

  if (flag->isZeroValue()) {
    val_ = CalcConstantExpr{}.Calc(node.rhs_);
  } else {
    val_ = CalcConstantExpr{}.Calc(node.lhs_);
  }
}

void CalcConstantExpr::Visit(const ConstantExpr& node) {
  if (node.GetType()->IsIntegerTy()) {
    val_ = llvm::ConstantInt::get(node.GetType()->GetLLVMType(),
                                  node.integer_val_);
  } else if (node.GetType()->IsFloatPointTy()) {
    val_ = llvm::ConstantFP::get(node.GetType()->GetLLVMType(),
                                 node.float_point_val_);
  } else {
    assert(false);
  }
}

void CalcConstantExpr::Visit(const EnumeratorExpr& node) {
  val_ = llvm::ConstantInt::get(node.GetType()->GetLLVMType(), node.val_);
}

void CalcConstantExpr::Visit(const StmtExpr& node) {
  if (!node.GetType()->IsVoidTy()) {
    auto last{node.block_->GetStmts().back()};
    assert(last->Kind() == AstNodeType::kExprStmt);
    val_ = CalcConstantExpr{}.Calc(dynamic_cast<ExprStmt*>(last)->GetExpr());
  } else {
    Error(node.GetLoc(), "expect constant expression");
  }
}

void CalcConstantExpr::Visit(const StringLiteralExpr& node) {
  auto width{node.GetType()->ArrayGetElementType()->GetWidth()};
  auto size{node.GetType()->ArrayGetNumElements()};
  auto temp{node.GetVal()};
  auto str{temp.c_str()};
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

  auto global_var{new llvm::GlobalVariable(*Module, arr->getType(), true,
                                           llvm::GlobalValue::PrivateLinkage,
                                           arr, ".str")};
  global_var->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  global_var->setAlignment(width);

  auto zero{llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), 0)};

  auto ptr{llvm::ConstantExpr::getInBoundsGetElementPtr(
      global_var->getValueType(), global_var,
      llvm::ArrayRef<llvm::Constant*>{zero, zero})};

  val_ = ptr;
}

void CalcConstantExpr::Visit(const FuncCallExpr& node) {
  Error(node.GetLoc(), "expect constant expression");
}

void CalcConstantExpr::Visit(const IdentifierExpr& node) {
  Error(node.GetLoc(), "expect constant expression");
}

void CalcConstantExpr::Visit(const ObjectExpr& node) {
  val_ = node.GetGlobalPtr();
}

void CalcConstantExpr::Visit(const LabelStmt&) { assert(false); }

void CalcConstantExpr::Visit(const CaseStmt&) { assert(false); }

void CalcConstantExpr::Visit(const DefaultStmt&) { assert(false); }

void CalcConstantExpr::Visit(const CompoundStmt&) { assert(false); }

void CalcConstantExpr::Visit(const ExprStmt&) { assert(false); }

void CalcConstantExpr::Visit(const IfStmt&) { assert(false); }

void CalcConstantExpr::Visit(const SwitchStmt&) { assert(false); }

void CalcConstantExpr::Visit(const WhileStmt&) { assert(false); }

void CalcConstantExpr::Visit(const DoWhileStmt&) { assert(false); }

void CalcConstantExpr::Visit(const ForStmt&) { assert(false); }

void CalcConstantExpr::Visit(const GotoStmt&) { assert(false); }

void CalcConstantExpr::Visit(const ContinueStmt&) { assert(false); }

void CalcConstantExpr::Visit(const BreakStmt&) { assert(false); }

void CalcConstantExpr::Visit(const ReturnStmt&) { assert(false); }

void CalcConstantExpr::Visit(const TranslationUnit&) { assert(false); }

void CalcConstantExpr::Visit(const Declaration&) { assert(false); }

void CalcConstantExpr::Visit(const FuncDef&) { assert(false); }

llvm::Constant* CalcConstantExpr::NegOp(llvm::Constant* value,
                                        bool is_unsigned) {
  if (value->getType()->isIntegerTy()) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getNeg(value);
    } else {
      return llvm::ConstantExpr::getNSWNeg(value);
    }
  } else if (value->getType()->isFloatingPointTy()) {
    return llvm::ConstantExpr::getFNeg(value);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* CalcConstantExpr::LogicNotOp(llvm::Constant* value) {
  value = ConstantCastToBool(value);
  value =
      llvm::ConstantExpr::getXor(value, llvm::ConstantInt::getTrue(Context));

  return ConstantCastTo(value, Builder.getInt32Ty(), true);
}

llvm::Constant* CalcConstantExpr::AddOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs, bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getAdd(lhs, rhs);
    } else {
      return llvm::ConstantExpr::getNSWAdd(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return llvm::ConstantExpr::getFAdd(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    return llvm::ConstantExpr::getInBoundsGetElementPtr(
        nullptr, lhs, llvm::ArrayRef<llvm::Constant*>{rhs});
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* CalcConstantExpr::SubOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs, bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getSub(lhs, rhs);
    } else {
      return llvm::ConstantExpr::getNSWSub(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return llvm::ConstantExpr::getFSub(lhs, rhs);
  } else if (IsPointerTy(lhs) && IsIntegerTy(rhs)) {
    return llvm::ConstantExpr::getInBoundsGetElementPtr(
        lhs->getType(), lhs,
        llvm::ArrayRef<llvm::Constant*>{NegOp(rhs, is_unsigned)});
  } else if (IsPointerTy(lhs) && IsPointerTy(rhs)) {
    auto type{lhs->getType()->getPointerElementType()};

    lhs = ConstantCastTo(lhs, Builder.getInt64Ty(), true);
    rhs = ConstantCastTo(rhs, Builder.getInt64Ty(), true);

    auto value{SubOp(lhs, rhs, true)};
    return DivOp(value, Builder.getInt64(DataLayout.getTypeAllocSize(type)),
                 true);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* CalcConstantExpr::MulOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs, bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getMul(lhs, rhs);
    } else {
      return llvm::ConstantExpr::getNSWMul(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return llvm::ConstantExpr::getFMul(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* CalcConstantExpr::DivOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs, bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getUDiv(lhs, rhs);
    } else {
      return llvm::ConstantExpr::getSDiv(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return llvm::ConstantExpr::getFDiv(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* CalcConstantExpr::ModOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs, bool is_unsigned) {
  if (is_unsigned) {
    return llvm::ConstantExpr::getURem(lhs, rhs);
  } else {
    return llvm::ConstantExpr::getSRem(lhs, rhs);
  }
}

llvm::Constant* CalcConstantExpr::OrOp(llvm::Constant* lhs,
                                       llvm::Constant* rhs) {
  return llvm::ConstantExpr::getOr(lhs, rhs);
}

llvm::Constant* CalcConstantExpr::AndOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs) {
  return llvm::ConstantExpr::getAnd(lhs, rhs);
}

llvm::Constant* CalcConstantExpr::XorOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs) {
  return llvm::ConstantExpr::getXor(lhs, rhs);
}

llvm::Constant* CalcConstantExpr::ShlOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs) {
  return llvm::ConstantExpr::getShl(lhs, rhs);
}

llvm::Constant* CalcConstantExpr::ShrOp(llvm::Constant* lhs,
                                        llvm::Constant* rhs, bool is_unsigned) {
  if (is_unsigned) {
    return llvm::ConstantExpr::getLShr(lhs, rhs);
  } else {
    return llvm::ConstantExpr::getAShr(lhs, rhs);
  }
}

llvm::Constant* CalcConstantExpr::LessEqualOp(llvm::Constant* lhs,
                                              llvm::Constant* rhs,
                                              bool is_unsigned) {
  llvm::Constant* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULE, lhs, rhs);
    } else {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SLE, lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OLE, lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULE, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return ConstantCastTo(value, Builder.getInt32Ty(), true);
}

llvm::Constant* CalcConstantExpr::LessOp(llvm::Constant* lhs,
                                         llvm::Constant* rhs,
                                         bool is_unsigned) {
  llvm::Constant* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULT, lhs, rhs);
    } else {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SLT, lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OLT, lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULT, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return ConstantCastTo(value, Builder.getInt32Ty(), true);
}

llvm::Constant* CalcConstantExpr::GreaterEqualOp(llvm::Constant* lhs,
                                                 llvm::Constant* rhs,
                                                 bool is_unsigned) {
  llvm::Constant* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGE, lhs, rhs);
    } else {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SGE, lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OGE, lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGE, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return ConstantCastTo(value, Builder.getInt32Ty(), true);
}

llvm::Constant* CalcConstantExpr::GreaterOp(llvm::Constant* lhs,
                                            llvm::Constant* rhs,
                                            bool is_unsigned) {
  llvm::Constant* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGT, lhs, rhs);
    } else {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SGT, lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OGT, lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGT, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return ConstantCastTo(value, Builder.getInt32Ty(), true);
}

llvm::Constant* CalcConstantExpr::EqualOp(llvm::Constant* lhs,
                                          llvm::Constant* rhs) {
  llvm::Constant* value{};

  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_EQ, lhs, rhs);
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OEQ, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return ConstantCastTo(value, Builder.getInt32Ty(), true);
}

llvm::Constant* CalcConstantExpr::NotEqualOp(llvm::Constant* lhs,
                                             llvm::Constant* rhs) {
  llvm::Constant* value{};

  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_NE, lhs, rhs);
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_ONE, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return ConstantCastTo(value, Builder.getInt32Ty(), true);
}

llvm::Constant* CalcConstantExpr::LogicOrOp(const BinaryOpExpr& node) {
  auto lhs{CalcConstantExpr{}.Calc(node.lhs_)};
  if (lhs->isZeroValue()) {
    auto rhs{CalcConstantExpr{}.Calc(node.rhs_)};
    return GetInt32Constant(!rhs->isZeroValue());
  } else {
    return GetInt32Constant(1);
  }
}

llvm::Constant* CalcConstantExpr::LogicAndOp(const BinaryOpExpr& node) {
  auto lhs{CalcConstantExpr{}.Calc(node.lhs_)};
  if (lhs->isZeroValue()) {
    return GetInt32Constant(0);
  } else {
    auto rhs{CalcConstantExpr{}.Calc(node.rhs_)};
    return GetInt32Constant(!rhs->isZeroValue());
  }
}

}  // namespace kcc
