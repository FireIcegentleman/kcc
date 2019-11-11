//
// Created by kaiser on 2019/10/30.
//

#include "cpp.h"

#include <memory>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/LangStandard.h>
#include <clang/Lex/DirectoryLookup.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/HeaderSearchOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>

//#include "error.h"

namespace kcc {

void Preprocessor::SetIncludePaths(
    const std::vector<std::string> &include_paths) {
  for (const auto &item : include_paths) {
    cmd_ += "-I" + item + " ";
  }
}

void Preprocessor::SetMacroDefinitions(
    const std::vector<std::string> &macro_definitions) {
  for (const auto &item : macro_definitions) {
    cmd_ += "-D" + item + " ";
  }
}

std::string Preprocessor::Cpp(const std::string &input_file) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();

  auto ci{std::make_unique<clang::CompilerInstance>()};

  ci->createDiagnostics();

  auto &lang_opts{ci->getLangOpts()};
  lang_opts.C17 = true;
  lang_opts.GNUMode = true;
  lang_opts.GNUKeywords = true;

  auto invocation{std::make_shared<clang::CompilerInvocation>()};
  llvm::Triple triple{llvm::sys::getDefaultTargetTriple()};
  invocation->setLangDefaults(lang_opts, clang::InputKind::C, triple,
                              ci->getPreprocessorOpts(),
                              clang::LangStandard::lang_c17);
  ci->setInvocation(invocation);

  auto pto{std::make_shared<clang::TargetOptions>()};
  pto->Triple = llvm::sys::getDefaultTargetTriple();

  std::shared_ptr<clang::TargetInfo> pti(
      clang::TargetInfo::CreateTargetInfo(ci->getDiagnostics(), pto));
  ci->setTarget(pti.get());

  ci->createFileManager();
  ci->createSourceManager(ci->getFileManager());
  ci->createPreprocessor(clang::TranslationUnitKind::TU_Complete);

  auto header_search_opts{std::make_shared<clang::HeaderSearchOptions>()};
  auto &pp{ci->getPreprocessor()};
  auto &header_search{pp.getHeaderSearchInfo()};

  header_search.AddSearchPath(
      clang::DirectoryLookup(ci->getFileManager().getDirectory("/usr/include"),
                             clang::SrcMgr::C_System, false),
      true);
  header_search.AddSearchPath(
      clang::DirectoryLookup(
          ci->getFileManager().getDirectory("/usr/local/include"),
          clang::SrcMgr::C_System, false),
      true);
  header_search.AddSearchPath(
      clang::DirectoryLookup(
          ci->getFileManager().getDirectory("/usr/lib/clang/9.0.0/include"),
          clang::SrcMgr::C_System, false),
      true);

  auto file{ci->getFileManager().getFile(input_file)};

  ci->getSourceManager().setMainFileID(ci->getSourceManager().createFileID(
      file, clang::SourceLocation(), clang::SrcMgr::C_User));

  pp.EnterMainSourceFile();
  ci->getDiagnosticClient().BeginSourceFile(ci->getLangOpts(),
                                            &ci->getPreprocessor());
  clang::Token tok;
  std::string code;
  do {
    pp.Lex(tok);
    if (ci->getDiagnostics().hasErrorOccurred()) {
      break;
    }

    code += pp.getSpelling(tok) += '\n';
  } while (tok.isNot(clang::tok::eof));

  ci->getDiagnosticClient().EndSourceFile();

  return code;
}

}  // namespace kcc
