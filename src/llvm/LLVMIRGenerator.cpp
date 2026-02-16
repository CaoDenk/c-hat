#include "LLVMIRGenerator.h"
#include <iostream>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

namespace c_hat::llvm_codegen {

LLVMIRGenerator::LLVMIRGenerator(const std::string &moduleName)
    : context(std::make_unique<llvm::LLVMContext>()),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)),
      module(std::make_unique<llvm::Module>(moduleName, *context)) {}

LLVMIRGenerator::~LLVMIRGenerator() = default;

bool LLVMIRGenerator::verifyIR() {
  std::string error;
  llvm::raw_string_ostream os(error);
  if (llvm::verifyModule(*module, &os)) {
    std::cerr << "IR verification failed: " << error << std::endl;
    return false;
  }
  return true;
}

void LLVMIRGenerator::printIR() { module->print(llvm::outs(), nullptr); }

bool LLVMIRGenerator::writeIRToFile(const std::string &filename) {
  std::error_code ec;
  llvm::raw_fd_ostream file(filename, ec, llvm::sys::fs::OF_None);

  if (ec) {
    std::cerr << "Error opening file: " << ec.message() << std::endl;
    return false;
  }

  module->print(file, nullptr);
  return true;
}

bool LLVMIRGenerator::emitObjectFile(const std::string &filename) {
  std::cerr << "Object file emission via LLVM API is temporarily disabled."
            << std::endl;
  std::cerr << "Please generate LLVM IR first (--emit-llvm), then use llc and "
               "link.exe manually."
            << std::endl;
  return false;
}

bool LLVMIRGenerator::emitAssemblyFile(const std::string &filename) {
  std::cerr << "Assembly file emission via LLVM API is temporarily disabled."
            << std::endl;
  std::cerr
      << "Please generate LLVM IR first (--emit-llvm), then use llc manually."
      << std::endl;
  return false;
}

} // namespace c_hat::llvm_codegen
