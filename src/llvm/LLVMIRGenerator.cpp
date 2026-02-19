#include "LLVMIRGenerator.h"
#include <iostream>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

namespace c_hat::llvm_codegen {

LLVMIRGenerator::LLVMIRGenerator(const std::string &moduleName)
    : context(std::make_unique<llvm::LLVMContext>()),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)),
      module(std::make_unique<llvm::Module>(moduleName, *context)) {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
}

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
  std::string error;
  auto targetTriple = llvm::sys::getDefaultTargetTriple();
  auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

  if (!target) {
    std::cerr << "Error looking up target: " << error << std::endl;
    return false;
  }

  llvm::TargetOptions opt;
  auto RM = std::optional<llvm::Reloc::Model>();
  auto targetMachine =
      target->createTargetMachine(targetTriple, "generic", "", opt, RM);

  module->setDataLayout(targetMachine->createDataLayout());
  module->setTargetTriple(targetTriple);

  std::error_code ec;
  llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

  if (ec) {
    std::cerr << "Error opening output file: " << ec.message() << std::endl;
    return false;
  }

  llvm::legacy::PassManager pass;
  auto fileType = llvm::CodeGenFileType::ObjectFile;

  if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
    std::cerr << "TargetMachine can't emit file of this type" << std::endl;
    return false;
  }

  pass.run(*module);
  return true;
}

bool LLVMIRGenerator::emitAssemblyFile(const std::string &filename) {
  std::string error;
  auto targetTriple = llvm::sys::getDefaultTargetTriple();
  auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

  if (!target) {
    std::cerr << "Error looking up target: " << error << std::endl;
    return false;
  }

  llvm::TargetOptions opt;
  auto RM = std::optional<llvm::Reloc::Model>();
  auto targetMachine =
      target->createTargetMachine(targetTriple, "generic", "", opt, RM);

  module->setDataLayout(targetMachine->createDataLayout());
  module->setTargetTriple(targetTriple);

  std::error_code ec;
  llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

  if (ec) {
    std::cerr << "Error opening output file: " << ec.message() << std::endl;
    return false;
  }

  llvm::legacy::PassManager pass;
  auto fileType = llvm::CodeGenFileType::AssemblyFile;

  if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
    std::cerr << "TargetMachine can't emit file of this type" << std::endl;
    return false;
  }

  pass.run(*module);
  return true;
}

} // namespace c_hat::llvm_codegen
