#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Target/TargetMachine.h>
#include <map>
#include <memory>
#include <string>

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

private:
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::unique_ptr<llvm::Module> module;
};

} // namespace c_hat::llvm_codegen
