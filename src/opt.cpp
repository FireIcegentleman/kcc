//
// Created by kaiser on 2019/11/2.
//

#include "opt.h"

#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include <memory>

namespace kcc {

extern std::unique_ptr<llvm::Module> Module;

extern std::unique_ptr<llvm::TargetMachine> TargetMachine;

void Optimization(OptLevel opt_level) {
  std::uint32_t level{};

  switch (opt_level) {
    case OptLevel::kO0:
      level = 0;
      break;
    case OptLevel::kO1:
      level = 1;
      break;
    case OptLevel::kO2:
      level = 2;
      break;
    case OptLevel::kO3:
      level = 3;
      break;
  }

  if (level != 0) {
    // 初始化
    llvm::PassRegistry &registry = *llvm::PassRegistry::getPassRegistry();

    llvm::initializeCore(registry);
    llvm::initializeCoroutines(registry);
    llvm::initializeScalarOpts(registry);
    llvm::initializeObjCARCOpts(registry);
    llvm::initializeVectorization(registry);
    llvm::initializeIPO(registry);
    llvm::initializeAnalysis(registry);
    llvm::initializeTransformUtils(registry);
    llvm::initializeInstCombine(registry);
    llvm::initializeAggressiveInstCombine(registry);
    llvm::initializeInstrumentation(registry);
    llvm::initializeTarget(registry);
    llvm::initializeExpandMemCmpPassPass(registry);
    llvm::initializeScalarizeMaskedMemIntrinPass(registry);
    llvm::initializeCodeGenPreparePass(registry);
    llvm::initializeAtomicExpandPass(registry);
    llvm::initializeRewriteSymbolsLegacyPassPass(registry);
    llvm::initializeWinEHPreparePass(registry);
    llvm::initializeDwarfEHPreparePass(registry);
    llvm::initializeSafeStackLegacyPassPass(registry);
    llvm::initializeSjLjEHPreparePass(registry);
    llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registry);
    llvm::initializeGlobalMergePass(registry);
    llvm::initializeIndirectBrExpandPassPass(registry);
    llvm::initializeInterleavedAccessPass(registry);
    llvm::initializeEntryExitInstrumenterPass(registry);
    llvm::initializePostInlineEntryExitInstrumenterPass(registry);
    llvm::initializeUnreachableBlockElimLegacyPassPass(registry);
    llvm::initializeExpandReductionsPass(registry);
    llvm::initializeWasmEHPreparePass(registry);

    auto fp_pass{
        std::make_unique<llvm::legacy::FunctionPassManager>(Module.get())};
    llvm::legacy::PassManager passes;

    fp_pass->add(llvm::createTargetTransformInfoWrapperPass(
        TargetMachine->getTargetIRAnalysis()));

    // 验证输入是否正确
    fp_pass->add(llvm::createVerifierPass());
    passes.add(llvm::createVerifierPass());

    llvm::PassManagerBuilder builder;
    builder.OptLevel = level;

    if (level > 1) {
      builder.Inliner = llvm::createFunctionInliningPass(level, 0, false);
    } else {
      builder.Inliner = llvm::createAlwaysInlinerLegacyPass();
    }

    TargetMachine->adjustPassManager(builder);

    builder.populateFunctionPassManager(*fp_pass);
    builder.populateModulePassManager(passes);

    fp_pass->doInitialization();
    for (auto &f : *Module) {
      fp_pass->run(f);
    }
    fp_pass->doFinalization();

    passes.run(*Module);
  }
}

}  // namespace kcc
