#include "LLVMIRGenerator.h"
#include <iostream>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
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

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace c_hat::llvm_codegen {

struct LLVMIRGenerator::JITState {
  std::unique_ptr<llvm::orc::LLJIT> jit;
  std::string errorMessage;

  JITState() {
    auto jitBuilder = llvm::orc::LLJITBuilder();
    auto jitOrErr = jitBuilder.create();
    if (jitOrErr) {
      jit = std::move(*jitOrErr);
    } else {
      llvm::handleAllErrors(
          jitOrErr.takeError(), [&](const llvm::ErrorInfoBase &ei) {
            errorMessage = ei.message();
            std::cerr << "JIT creation failed: " << errorMessage << std::endl;
          });
    }
  }

  bool isValid() const { return jit != nullptr; }
};

LLVMIRGenerator::LLVMIRGenerator(const std::string &moduleName)
    : context(std::make_unique<llvm::LLVMContext>()),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)),
      module(std::make_unique<llvm::Module>(moduleName, *context)) {
  // 初始化 LLVM 目标（必须在创建 JIT 之前）
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  // 初始化 JIT（必须在初始化 native target 之后）
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // 创建 JIT
  jit = std::make_unique<JITState>();
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

void LLVMIRGenerator::addExternalSymbol(const std::string &name,
                                        void *address) {
  if (!jit || !jit->isValid()) {
    std::cerr << "JIT not initialized" << std::endl;
    return;
  }

  auto &mainDylib = jit->jit->getMainJITDylib();

  llvm::orc::SymbolMap symbols;
  symbols[jit->jit->mangleAndIntern(name)] =
      llvm::orc::ExecutorSymbolDef(llvm::orc::ExecutorAddr::fromPtr(address),
                                   llvm::JITSymbolFlags::Exported);

  if (auto err =
          mainDylib.define(llvm::orc::absoluteSymbols(std::move(symbols)))) {
    llvm::consumeError(std::move(err));
    std::cerr << "Failed to add external symbol: " << name << std::endl;
  }
}

int LLVMIRGenerator::runJIT(const std::string &entryPoint) {
  if (!jit || !jit->isValid()) {
    std::cerr << "JIT not initialized" << std::endl;
    return -1;
  }

  auto tsModule =
      llvm::orc::ThreadSafeModule(std::move(module), std::move(context));

  if (auto err = jit->jit->addIRModule(std::move(tsModule))) {
    std::cerr << "Failed to add module to JIT: ";
    llvm::consumeError(std::move(err));
    return -1;
  }

  auto mainSymbol = jit->jit->lookup(entryPoint);
  if (!mainSymbol) {
    std::cerr << "Failed to find entry point: " << entryPoint << std::endl;
    llvm::consumeError(mainSymbol.takeError());
    return -1;
  }

  auto *mainFn = (int (*)())mainSymbol->toPtr<int (*)()>();

  return mainFn();
}

} // namespace c_hat::llvm_codegen
