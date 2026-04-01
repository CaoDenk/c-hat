#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Target/TargetMachine.h>
#include <map>
#include <memory>
#include <string>
#include <functional>

namespace c_hat::llvm_codegen {

class LLVMIRGenerator {
public:
  explicit LLVMIRGenerator(const std::string &moduleName);
  ~LLVMIRGenerator();

  llvm::Module *getModule() { return module.get(); }
  llvm::IRBuilder<> *getBuilder() { return builder.get(); }
  llvm::LLVMContext *getContext() { return context.get(); }

  bool verifyIR();
  void printIR();
  bool writeIRToFile(const std::string &filename);

  // 生成目标文件
  bool emitObjectFile(const std::string &filename);

  // 生成汇编文件
  bool emitAssemblyFile(const std::string &filename);

  // JIT 执行
  bool hasJIT() const { return jit != nullptr; }
  int runJIT(const std::string &entryPoint = "main");
  
  // 添加外部函数符号（用于调用 C 库函数）
  void addExternalSymbol(const std::string &name, void *address);

private:
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::unique_ptr<llvm::Module> module;
  
  // JIT 相关
  struct JITState;
  std::unique_ptr<JITState> jit;
};

} // namespace c_hat::llvm_codegen
