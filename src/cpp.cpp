//
// Created by kaiser on 2019/10/30.
//

#include "cpp.h"

#include <memory>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/LangStandard.h>
#include <clang/Frontend/PreprocessorOutputOptions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/DirectoryLookup.h>
#include <fmt/format.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/Triple.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include "error.h"
#include "llvm_common.h"

namespace kcc {

Preprocessor::Preprocessor() {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargetMCs();

  ci_.createDiagnostics();

  auto pto{std::make_shared<clang::TargetOptions>()};
  pto->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo = clang::TargetInfo::CreateTargetInfo(ci_.getDiagnostics(), pto);

  ci_.setTarget(TargetInfo);

  ci_.getInvocation().setLangDefaults(
      ci_.getLangOpts(), clang::InputKind::C, llvm::Triple{pto->Triple},
      ci_.getPreprocessorOpts(), clang::LangStandard::lang_c17);

  auto &lang_opt{ci_.getLangOpts()};
  lang_opt.C17 = true;
  lang_opt.Digraphs = true;
  lang_opt.Trigraphs = true;
  lang_opt.GNUMode = true;
  lang_opt.GNUKeywords = true;

  ci_.createFileManager();
  ci_.createSourceManager(ci_.getFileManager());

  ci_.createPreprocessor(clang::TranslationUnitKind::TU_Complete);

  pp_ = &ci_.getPreprocessor();
  header_search_ = &pp_->getHeaderSearchInfo();

  AddIncludePath("/usr/include", true);
  AddIncludePath("/usr/local/include", true);

  /*
   * Platform Specific Code
   */
  AddIncludePath("/usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/include", true);
  AddIncludePath("/usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/include-fixed", true);
  /*
   * End of Platform Specific Code
   */

  pp_->setPredefines(pp_->getPredefines() +
                     "#define __KCC__ 1\n"
                     "#define __STDC_NO_ATOMICS__ 1\n"
                     "#define __STDC_NO_COMPLEX__ 1\n"
                     "#define __STDC_NO_THREADS__ 1\n"
                     "#define __STDC_NO_VLA__ 1\n"
                     "#define __builtin_va_arg(args,type) "
                     "  *(type*)__builtin_va_arg_sub(args,type)\n");
}

void Preprocessor::SetIncludePaths(
    const std::vector<std::string> &include_paths) {
  for (const auto &path : include_paths) {
    AddIncludePath(path, false);
  }
}

void Preprocessor::SetMacroDefinitions(
    const std::vector<std::string> &macro_definitions) {
  for (const auto &macro : macro_definitions) {
    auto index{macro.find_first_of('=')};
    auto name{macro.substr(0, index)};
    auto value{macro.substr(index + 1)};

    pp_->setPredefines(pp_->getPredefines() +
                       fmt::format(fmt("#define {} {}\n"), name, value));
  }
}

std::string Preprocessor::Cpp(const std::string &input_file) {
  Module = std::make_unique<llvm::Module>(input_file, Context);

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

  auto file{ci_.getFileManager().getFile(input_file)};

  ci_.getSourceManager().setMainFileID(ci_.getSourceManager().createFileID(
      file, clang::SourceLocation(), clang::SrcMgr::C_User));
  ci_.getDiagnosticClient().BeginSourceFile(ci_.getLangOpts(), pp_);

  std::string code;
  code.reserve(Preprocessor::StrReserve);
  llvm::raw_string_ostream os{code};

  clang::PreprocessorOutputOptions opts;
  opts.ShowCPP = true;

  clang::DoPrintPreprocessedInput(*pp_, &os, opts);
  os.flush();

  if (ci_.getDiagnostics().hasErrorOccurred()) {
    Error("Preprocess failure");
  }

  ci_.getDiagnosticClient().EndSourceFile();

  return code;
}

void Preprocessor::AddIncludePath(const std::string &path, bool is_system) {
  if (is_system) {
    clang::DirectoryLookup directory{ci_.getFileManager().getDirectory(path),
                                     clang::SrcMgr::C_System, false};
    header_search_->AddSearchPath(directory, true);
  } else {
    clang::DirectoryLookup directory{ci_.getFileManager().getDirectory(path),
                                     clang::SrcMgr::C_User, false};
    header_search_->AddSearchPath(directory, false);
  }
}

}  // namespace kcc
