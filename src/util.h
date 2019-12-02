//
// Created by kaiser on 2019/11/9.
//

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <llvm/Support/CommandLine.h>

#include "llvm_common.h"

namespace kcc {

inline llvm::cl::OptionCategory Category{"Compiler Options"};

inline llvm::cl::list<std::string> InputFilePaths{
    llvm::cl::desc{"files"}, llvm::cl::Positional, llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> OutputObjectFile{
    "c", llvm::cl::desc{"Only run preprocess, compile, and assemble steps"},
    llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> MacroDefines{
    "D", llvm::cl::desc{"Define <macro> to <value> (or 1 if <value> omitted)"},
    llvm::cl::value_desc{"macro>=<value"}, llvm::cl::Prefix,
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitTokens{
    "emit-tokens", llvm::cl::desc{"Emit kcc tokens for source inputs"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitAST{
    "emit-ast", llvm::cl::desc{"Emit kcc AST for source inputs"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitLLVM{
    "emit-llvm",
    llvm::cl::desc{
        "Use the LLVM representation for assembler and object files"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Preprocess{
    "E", llvm::cl::desc{"Only run the preprocessor"}, llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> IncludePaths{
    "I", llvm::cl::desc{"Add directory to include search path"},
    llvm::cl::value_desc{"dir"}, llvm::cl::Prefix, llvm::cl::cat{Category}};

inline llvm::cl::opt<std::string> OutputFilePath{
    "o", llvm::cl::desc("Write output to <file>"), llvm::cl::value_desc("file"),
    llvm::cl::cat{Category}};

inline llvm::cl::opt<OptLevel> OptimizationLevel{
    llvm::cl::desc{"Set optimization level to <number>"},
    llvm::cl::value_desc{"number"}, llvm::cl::init(OptLevel::kO2),
    llvm::cl::values(
        clEnumValN(OptLevel::kO0, "O0", "No optimizations"),
        clEnumValN(OptLevel::kO1, "O1", "Enable trivial optimizations"),
        clEnumValN(OptLevel::kO2, "O2", "Enable default optimizations"),
        clEnumValN(OptLevel::kO3, "O3", "Enable expensive optimizations")),
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> OutputAssembly{
    "S", llvm::cl::desc{"Only run preprocess and compilation steps"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Timing{"t", llvm::cl::desc{"Timing"},
                                  llvm::cl::cat{Category}};

#ifdef DEV
inline llvm::cl::opt<bool> TestMode{"test", llvm::cl::desc{"Test Mode"},
                                    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> DevMode{"dev", llvm::cl::desc{"Dev Mode"},
                                   llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Test8cc{"8cc", llvm::cl::desc{"Test 8cc"},
                                   llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> ParseOnly{"p", llvm::cl::desc{"Parse Only"},
                                     llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> StandardIR{"ir", llvm::cl::desc{"emit Standard IR"},
                                      llvm::cl::cat{Category}};
#endif

std::string GetPath();

void TimingStart();

void TimingEnd(const std::string &str = "");

void InitCommandLine(int argc, char *argv[]);

void CommandLineCheck();

void EnsureFileExists(const std::string &file_name);

std::string GetObjFile(const std::string &name);

std::vector<std::string> GetObjFiles();

std::string GetFileName(const std::string &name, std::string_view extension);

void RemoveAllFiles(const std::vector<std::string> &files);

bool CommandSuccess(std::int32_t status);

}  // namespace kcc
