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

enum class OptLevel { kO0, kO1, kO2, kO3 };

inline std::vector<std::string> SoFile;

inline std::vector<std::string> AFile;

inline std::vector<std::string> ObjFile;

inline std::vector<std::string> RemoveFile;

inline llvm::cl::OptionCategory Category{"Compiler Options"};

inline llvm::cl::list<std::string> InputFilePaths{
    llvm::cl::desc{"<input files>"}, llvm::cl::Positional,
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> OutputObjectFile{
    "c", llvm::cl::desc{"Only run preprocess, compile, and assemble steps"},
    llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> MacroDefines{
    "D", llvm::cl::desc{"Predefine the specified macro"},
    llvm::cl::value_desc{"macro"}, llvm::cl::Prefix, llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitTokens{
    "emit-tokens",
    llvm::cl::desc{"Run preprocessor, dump internal rep of tokens"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitAST{
    "emit-ast", llvm::cl::desc{"Build ASTs and then debug dump them"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitLLVM{
    "emit-llvm",
    llvm::cl::desc{"Build ASTs then convert to LLVM, emit .ll file"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Preprocess{
    "E", llvm::cl::desc{"Run preprocessor, emit preprocessed file"},
    llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> IncludePaths{
    "I", llvm::cl::desc{"Add directory to include search path"},
    llvm::cl::value_desc{"directory"}, llvm::cl::Prefix,
    llvm::cl::cat{Category}};

inline llvm::cl::opt<std::string> OutputFilePath{
    "o", llvm::cl::desc("Specify output file"), llvm::cl::value_desc("path"),
    llvm::cl::cat{Category}};

inline llvm::cl::opt<OptLevel> OptimizationLevel{
    llvm::cl::desc{"Set optimization level to <number>"},
    llvm::cl::value_desc{"number"}, llvm::cl::init(OptLevel::kO2),
    llvm::cl::values(
        clEnumValN(OptLevel::kO0, "O0", "No optimizations"),
        clEnumValN(OptLevel::kO1, "O1", "Enable trivial optimizations"),
        clEnumValN(OptLevel::kO2, "O2",
                   "Enable default optimizations (default)"),
        clEnumValN(OptLevel::kO3, "O3", "Enable expensive optimizations")),
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> OutputAssembly{
    "S", llvm::cl::desc{"Emit native assembly code"}, llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Timing{
    "t", llvm::cl::desc{"Print the amount of time"}, llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Shared{"shared",
                                  llvm::cl::desc{"Generate dynamic library"},
                                  llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Static{"static",
                                  llvm::cl::desc{"Generate static library"},
                                  llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> RPath{
    "rpath", llvm::cl::desc{"Add a DT_RUNPATH to the output"},
    llvm::cl::value_desc{"directory"}, llvm::cl::Prefix,
    llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> Libs{
    "l", llvm::cl::desc{"Add library"}, llvm::cl::value_desc{"file"},
    llvm::cl::Prefix, llvm::cl::cat{Category}};

#ifdef DEV
inline llvm::cl::opt<bool> TestMode{"test", llvm::cl::desc{"Test Mode"},
                                    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> DevMode{"dev", llvm::cl::desc{"Dev Mode"},
                                   llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> SingleMode{"single", llvm::cl::desc{"Single Mode"},
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

const std::vector<std::string> &GetObjFiles();

std::string GetFileName(const std::string &name, std::string_view extension);

void RemoveAllFiles(const std::vector<std::string> &files);

bool CommandSuccess(std::int32_t status);

}  // namespace kcc
