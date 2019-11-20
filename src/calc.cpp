//
// Created by kaiser on 2019/11/17.
//

#include "calc.h"

#include "util.h"

namespace kcc {

llvm::Constant* ConstantInitExpr::Calc(Expr* expr) {
  expr->Accept(*this);

  assert(val_ != nullptr);
  return val_;
}

llvm::Constant* NegOp(llvm::Constant* value, bool is_unsigned) {
  if (value->getType()->isIntegerTy()) {
    return llvm::ConstantExpr::getNeg(value, false, !is_unsigned);
  } else if (value->getType()->isFloatingPointTy()) {
    return llvm::ConstantExpr::getFNeg(value);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* LogicNotOp(llvm::Constant* value) {
  value = ConstantCastToBool(value);
  value =
      llvm::ConstantExpr::getXor(value, llvm::ConstantInt::getTrue(Context));

  return ConstantCastTo(value, Builder.getInt32Ty(), true);
}

void ConstantInitExpr::Visit(const UnaryOpExpr& node) {
  switch (node.op_) {
    case Tag::kPlus:
      val_ = ConstantInitExpr{}.Calc(node.expr_);
      break;
    case Tag::kMinus:
      val_ = NegOp(ConstantInitExpr{}.Calc(node.expr_),
                   node.GetType()->IsUnsigned());
      break;
    case Tag::kTilde:
      val_ = llvm::ConstantExpr::getNot(ConstantInitExpr{}.Calc(node.expr_));
      break;
    case Tag::kExclaim:
      val_ = LogicNotOp(ConstantInitExpr{}.Calc(node.expr_));
      break;
    case Tag::kAmp: {
      // 运算对象只能是一个全局变量
      auto p{dynamic_cast<ObjectExpr*>(node.expr_)};
      assert(p != nullptr);
      val_ = p->global_ptr_;
    } break;
    default:
      Error(node.expr_, "expect constant expression");
  }
}

void ConstantInitExpr::Visit(const TypeCastExpr& node) {
  //  if (node.expr_->GetType()->IsArithmeticTy() && node.to_->IsIntegerTy()) {
  //    val_ = ConstantInitExpr{}.Calc(node.expr_);
  //  } else {
  //    Error(node.expr_, "expect constant expression");
  //  }
  val_ = ConstantCastTo(ConstantInitExpr{}.Calc(node.expr_),
                        node.to_->GetLLVMType(), node.GetType()->IsUnsigned());
}

void ConstantInitExpr::Visit(const BinaryOpExpr& node) {
#define L ConstantInitExpr{}.Calc(node.lhs_)
#define R ConstantInitExpr{}.Calc(node.rhs_)

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
      Error(node.lhs_, "expect constant expression");
  }

#undef L
#undef R
#undef I_L
#undef I_R
}

void ConstantInitExpr::Visit(const ConditionOpExpr& node) {
  auto flag{ConstantInitExpr{}.Calc(node.cond_)};
  auto lhs{ConstantInitExpr{}.Calc(node.lhs_)};
  auto rhs{ConstantInitExpr{}.Calc(node.rhs_)};

  val_ = llvm::ConstantExpr::getSelect(flag, lhs, rhs);
}

void ConstantInitExpr::Visit(const ConstantExpr& node) {
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

void ConstantInitExpr::Visit(const EnumeratorExpr& node) {
  val_ = llvm::ConstantInt::get(node.GetType()->GetLLVMType(), node.val_);
}

void ConstantInitExpr::Visit(const StmtExpr& node) {
  if (!node.GetType()->IsVoidTy()) {
    auto last{node.block_->GetStmts().back()};
    assert(last->Kind() == AstNodeType::kExprStmt);
    val_ = ConstantInitExpr{}.Calc(dynamic_cast<ExprStmt*>(last)->GetExpr());
  } else {
    Error(node.GetLoc(), "expect constant expression");
  }
}

void ConstantInitExpr::Visit(const StringLiteralExpr& node) {
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

void ConstantInitExpr::Visit(const FuncCallExpr& node) {
  Error(node.GetLoc(), "expect constant expression");
}

void ConstantInitExpr::Visit(const IdentifierExpr& node) {
  Error(node.GetLoc(), "expect constant expression");
}

void ConstantInitExpr::Visit(const ObjectExpr& node) {
  assert(node.global_ptr_ != nullptr);
  val_ = node.global_ptr_;
}

void ConstantInitExpr::Visit(const LabelStmt&) { assert(false); }

void ConstantInitExpr::Visit(const CaseStmt&) { assert(false); }

void ConstantInitExpr::Visit(const DefaultStmt&) { assert(false); }

void ConstantInitExpr::Visit(const CompoundStmt&) { assert(false); }

void ConstantInitExpr::Visit(const ExprStmt&) { assert(false); }

void ConstantInitExpr::Visit(const IfStmt&) { assert(false); }

void ConstantInitExpr::Visit(const SwitchStmt&) { assert(false); }

void ConstantInitExpr::Visit(const WhileStmt&) { assert(false); }

void ConstantInitExpr::Visit(const DoWhileStmt&) { assert(false); }

void ConstantInitExpr::Visit(const ForStmt&) { assert(false); }

void ConstantInitExpr::Visit(const GotoStmt&) { assert(false); }

void ConstantInitExpr::Visit(const ContinueStmt&) { assert(false); }

void ConstantInitExpr::Visit(const BreakStmt&) { assert(false); }

void ConstantInitExpr::Visit(const ReturnStmt&) { assert(false); }

void ConstantInitExpr::Visit(const TranslationUnit&) { assert(false); }

void ConstantInitExpr::Visit(const Declaration&) { assert(false); }

void ConstantInitExpr::Visit(const FuncDef&) { assert(false); }

llvm::Constant* ConstantCastTo(llvm::Constant* value, llvm::Type* to,
                               bool is_unsigned) {
  if (to->isIntegerTy(1)) {
    return ConstantCastToBool(value);
  }

  if (IsIntegerTy(value) && to->isIntegerTy()) {
    if (value->getType()->getIntegerBitWidth() > to->getIntegerBitWidth()) {
      return llvm::ConstantExpr::getTrunc(value, to);
    } else {
      if (is_unsigned) {
        return llvm::ConstantExpr::getZExt(value, to);
      } else {
        return llvm::ConstantExpr::getSExt(value, to);
      }
    }
  } else if (IsIntegerTy(value) && to->isFloatingPointTy()) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getUIToFP(value, to);
    } else {
      return llvm::ConstantExpr::getSIToFP(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isIntegerTy()) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getFPToUI(value, to);
    } else {
      return llvm::ConstantExpr::getFPToSI(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isFloatingPointTy()) {
    if (FloatPointRank(value->getType()) > FloatPointRank(to)) {
      return llvm::ConstantExpr::getFPTrunc(value, to);
    } else {
      return llvm::ConstantExpr::getFPExtend(value, to);
    }
  } else if (IsPointerTy(value) && to->isIntegerTy()) {
    return llvm::ConstantExpr::getPtrToInt(value, to);
  } else if (IsIntegerTy(value) && to->isPointerTy()) {
    return llvm::ConstantExpr::getIntToPtr(value, to);
  } else if (to->isVoidTy() || value->getType() == to) {
    return value;
  } else if (IsArrCastToPtr(value, to)) {
    auto zero{llvm::ConstantInt::get(Builder.getInt64Ty(), 0)};
    return llvm::ConstantExpr::getInBoundsGetElementPtr(
        nullptr, value, llvm::ArrayRef<llvm::Constant*>{zero, zero});
  } else if (IsPointerTy(value) && to->isPointerTy()) {
    return llvm::ConstantExpr::getPointerCast(value, to);
  } else {
    Error("{} to {}", LLVMTypeToStr(value->getType()), LLVMTypeToStr(to));
  }
}

llvm::Constant* ConstantCastToBool(llvm::Constant* value) {
  if (value->getType()->isIntegerTy(1)) {
    return value;
  }

  if (IsIntegerTy(value) || IsPointerTy(value)) {
    return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_NE, value,
                                       GetZero(value->getType()));
  } else if (IsFloatingPointTy(value)) {
    return llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_ONE, value,
                                       GetZero(value->getType()));
  } else {
    Error("{}", LLVMTypeToStr(value->getType()));
  }
}

llvm::Constant* AddOp(llvm::Constant* lhs, llvm::Constant* rhs,
                      bool is_unsigned) {
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

llvm::Constant* SubOp(llvm::Constant* lhs, llvm::Constant* rhs,
                      bool is_unsigned) {
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

llvm::Constant* MulOp(llvm::Constant* lhs, llvm::Constant* rhs,
                      bool is_unsigned) {
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

llvm::Constant* DivOp(llvm::Constant* lhs, llvm::Constant* rhs,
                      bool is_unsigned) {
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

llvm::Constant* ModOp(llvm::Constant* lhs, llvm::Constant* rhs,
                      bool is_unsigned) {
  if (is_unsigned) {
    return llvm::ConstantExpr::getURem(lhs, rhs);
  } else {
    return llvm::ConstantExpr::getSRem(lhs, rhs);
  }
}

llvm::Constant* OrOp(llvm::Constant* lhs, llvm::Constant* rhs) {
  return llvm::ConstantExpr::getOr(lhs, rhs);
}

llvm::Constant* AndOp(llvm::Constant* lhs, llvm::Constant* rhs) {
  return llvm::ConstantExpr::getAnd(lhs, rhs);
}

llvm::Constant* XorOp(llvm::Constant* lhs, llvm::Constant* rhs) {
  return llvm::ConstantExpr::getXor(lhs, rhs);
}

llvm::Constant* ShlOp(llvm::Constant* lhs, llvm::Constant* rhs) {
  return llvm::ConstantExpr::getShl(lhs, rhs);
}

llvm::Constant* ShrOp(llvm::Constant* lhs, llvm::Constant* rhs,
                      bool is_unsigned) {
  if (is_unsigned) {
    return llvm::ConstantExpr::getLShr(lhs, rhs);
  } else {
    return llvm::ConstantExpr::getAShr(lhs, rhs);
  }
}

llvm::Constant* LessEqualOp(llvm::Constant* lhs, llvm::Constant* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{
          llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULE, lhs, rhs)};
      return ConstantCastTo(value, Builder.getInt32Ty(), true);
    } else {
      auto value{
          llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SLE, lhs, rhs)};
      return ConstantCastTo(value, Builder.getInt32Ty(), true);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OLE, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else if (IsPointerTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULE, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* LessOp(llvm::Constant* lhs, llvm::Constant* rhs,
                       bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{
          llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULT, lhs, rhs)};
      return ConstantCastTo(value, Builder.getInt32Ty(), true);
    } else {
      auto value{
          llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SLT, lhs, rhs)};
      return ConstantCastTo(value, Builder.getInt32Ty(), true);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OLT, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else if (IsPointerTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULT, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* GreaterEqualOp(llvm::Constant* lhs, llvm::Constant* rhs,
                               bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{
          llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGE, lhs, rhs)};
      return ConstantCastTo(value, Builder.getInt32Ty(), true);
    } else {
      auto value{
          llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SGE, lhs, rhs)};
      return ConstantCastTo(value, Builder.getInt32Ty(), true);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OGE, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else if (IsPointerTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGE, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* GreaterOp(llvm::Constant* lhs, llvm::Constant* rhs,
                          bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{
          llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGT, lhs, rhs)};
      return ConstantCastTo(value, Builder.getInt32Ty(), true);
    } else {
      auto value{
          llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SGT, lhs, rhs)};
      return ConstantCastTo(value, Builder.getInt32Ty(), true);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OGT, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else if (IsPointerTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGT, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* EqualOp(llvm::Constant* lhs, llvm::Constant* rhs) {
  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_EQ, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else if (IsFloatingPointTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_OEQ, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* NotEqualOp(llvm::Constant* lhs, llvm::Constant* rhs) {
  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_NE, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else if (IsFloatingPointTy(lhs)) {
    auto value{llvm::ConstantExpr::getICmp(llvm::CmpInst::FCMP_ONE, lhs, rhs)};
    return ConstantCastTo(value, Builder.getInt32Ty(), true);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant* LogicOrOp(const BinaryOpExpr&) {
  // TODO
  return nullptr;
}

llvm::Constant* LogicAndOp(const BinaryOpExpr&) {
  // TODO
  return nullptr;
}

bool IsIntegerTy(llvm::Constant* value) {
  return value->getType()->isIntegerTy();
}

bool IsFloatingPointTy(llvm::Constant* value) {
  return value->getType()->isFloatingPointTy();
}

bool IsPointerTy(llvm::Constant* value) {
  return value->getType()->isPointerTy();
}

bool IsArrCastToPtr(llvm::Constant* value, llvm::Type* type) {
  auto value_type{value->getType()};

  return value_type->isPointerTy() &&
         value_type->getPointerElementType()->isArrayTy() &&
         type->isPointerTy() &&
         value_type->getPointerElementType()
                 ->getArrayElementType()
                 ->getPointerTo() == type;
}

std::int32_t FloatPointRank(llvm::Type* type) {
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

std::string LLVMTypeToStr(llvm::Type* type) {
  std::string s;
  llvm::raw_string_ostream rso{s};
  type->print(rso);
  return rso.str();
}

llvm::Constant* GetZero(llvm::Type* type) {
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

}  // namespace kcc
