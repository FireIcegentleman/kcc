//
// Created by kaiser on 2019/11/9.
//

#ifndef KCC_SRC_UTIL_H_
#define KCC_SRC_UTIL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Target/TargetMachine.h>

namespace kcc {

// 拥有许多 LLVM 核心数据结构, 如类型和常量值表
inline llvm::LLVMContext Context;
// 一个辅助对象, 跟踪当前位置并且可以插入 LLVM 指令
inline llvm::IRBuilder<> Builder{Context};
// 包含函数和全局变量, 它拥有生成的所有 IR 的内存
inline auto Module{std::make_unique<llvm::Module>("main", Context)};

inline llvm::DataLayout DataLayout{Module.get()};

inline std::unique_ptr<llvm::TargetMachine> TargetMachine;

enum class OptLevel { kO0, kO1, kO2, kO3 };

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
    "emit-ast", llvm::cl::desc{"Emit tcc AST for source inputs"},
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

inline llvm::cl::opt<bool> DevMode{"dev", llvm::cl::desc{"Dev Mode"},
                                   llvm::cl::cat{Category}};

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

}  // namespace kcc

#endif  // KCC_SRC_UTIL_H_
