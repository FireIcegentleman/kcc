//
// Created by kaiser on 2019/10/30.
//

#include "cpp.h"

#include <memory>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/LangStandard.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/DirectoryLookup.h>
#include <clang/Lex/HeaderSearch.h>
#include <fmt/format.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>

#ifdef DEV
#include "util.h"
#endif

namespace kcc {

Preprocessor::Preprocessor() {
  // 初始化
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();

  ci_.createDiagnostics();

  auto pto{std::make_shared<clang::TargetOptions>()};
  pto->Triple = llvm::sys::getDefaultTargetTriple();

  auto pti{clang::TargetInfo::CreateTargetInfo(ci_.getDiagnostics(), pto)};
  ci_.setTarget(pti);

  clang::LangOptions lang_options;
  lang_options.C17 = true;
  lang_options.Trigraphs = true;

  ci_.getInvocation().setLangDefaults(
      lang_options, clang::InputKind::C, llvm::Triple{pto->Triple},
      ci_.getPreprocessorOpts(), clang::LangStandard::lang_c17);

  ci_.createFileManager();
  ci_.createSourceManager(ci_.getFileManager());

  ci_.createPreprocessor(clang::TranslationUnitKind::TU_Complete);

  pp_ = &ci_.getPreprocessor();
  auto &header_search{pp_->getHeaderSearchInfo()};

  header_search.AddSearchPath(
      clang::DirectoryLookup(
          ci_.getFileManager().getDirectory(
              "/usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/include"),
          clang::SrcMgr::C_System, false),
      true);
  header_search.AddSearchPath(
      clang::DirectoryLookup(
          ci_.getFileManager().getDirectory("/usr/local/include"),
          clang::SrcMgr::C_System, false),
      true);
  header_search.AddSearchPath(
      clang::DirectoryLookup(
          ci_.getFileManager().getDirectory(
              "/usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/include-fixed"),
          clang::SrcMgr::C_System, false),
      true);
  header_search.AddSearchPath(
      clang::DirectoryLookup(ci_.getFileManager().getDirectory("/usr/include"),
                             clang::SrcMgr::C_System, false),
      true);

  pp_->setPredefines(
      pp_->getPredefines() +
      "#define __builtin_va_copy(dest,src) ((dest)[0]=(src)[0])\n"
      "#define __builtin_va_arg(ap, type)          \\\n"
      "  ({int klass = __builtin_reg_class((type *)0);  \\\n"
      "    *(type *)(klass == 0 ? __va_arg_gp(ap)   \\\n"
      "   : klass == 1 ? __va_arg_fp(ap) : __va_arg_mem(ap));   \\\n"
      "  })\n"
      "#define DECIMAL_DIG 21\n"
      "#define FLT_EVAL_METHOD 0\n"
      "#define FLT_TRUE_MIN 0x1p-149\n"
      "#define DBL_TRUE_MIN 0x0.0000000000001p-1022\n"
      "#define __STDC_VERSION__ 201710L\n");
}

void Preprocessor::SetIncludePaths(
    const std::vector<std::string> &include_paths) {
  for (const auto &path : include_paths) {
    pp_->getHeaderSearchInfo().AddSearchPath(
        clang::DirectoryLookup(ci_.getFileManager().getDirectory(path),
                               clang::SrcMgr::C_User, false),
        false);
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
  auto file{ci_.getFileManager().getFile(input_file)};

  ci_.getSourceManager().setMainFileID(ci_.getSourceManager().createFileID(
      file, clang::SourceLocation(), clang::SrcMgr::C_User));
  ci_.getDiagnosticClient().BeginSourceFile(ci_.getLangOpts(), pp_);

  std::string code;
  code.reserve(4096);

  llvm::raw_string_ostream os{code};
  clang::PreprocessorOutputOptions opts;
  opts.ShowCPP = true;

  clang::DoPrintPreprocessedInput(*pp_, &os, opts);
  os.flush();

  ci_.getDiagnosticClient().EndSourceFile();

#ifdef DEV
  if (NoBuiltin) {
    return code;
  } else {
    return Preprocessor::Builtin + code;
  }
#else
  return Preprocessor::Builtin + code;
#endif
}

}  // namespace kcc
