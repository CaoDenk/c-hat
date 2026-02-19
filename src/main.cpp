#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"
#include "llvm/LLVMCodeGenerator.h"
#include <argparse/argparse.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <vector>

// LLD headers
#include <lld/Common/Driver.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/raw_ostream.h>

// Declare that we use the COFF driver
LLD_HAS_DRIVER(coff);
LLD_HAS_DRIVER(elf);
LLD_HAS_DRIVER(mingw);
LLD_HAS_DRIVER(macho);
LLD_HAS_DRIVER(wasm);

namespace fs = std::filesystem;

fs::path modulePathToFilePath(const std::vector<std::string> &modulePath,
                              const std::string &stdlibPath) {
  fs::path basePath;
  if (!stdlibPath.empty()) {
    basePath = stdlibPath;
  } else {
    basePath = "stdlib";
  }

  for (const auto &part : modulePath) {
    basePath /= part;
  }

  auto filePath = basePath;
  filePath += ".ch";

  return filePath;
}

int main(int argc, char *argv[]) {
  argparse::ArgumentParser argParser("C^ Compiler (chc)");

  argParser.add_argument("input-file").help("Input source file");
  argParser.add_argument("-o", "--output")
      .help("Output file name")
      .default_value(std::string(""));
  argParser.add_argument("--dump-ast")
      .help("Dump the abstract syntax tree")
      .default_value(false)
      .implicit_value(true);
  argParser.add_argument("--dump-tokens")
      .help("Dump the lexer tokens")
      .default_value(false)
      .implicit_value(true);
  argParser.add_argument("--dump-ir")
      .help("Dump the LLVM IR")
      .default_value(false)
      .implicit_value(true);
  argParser.add_argument("--emit-llvm")
      .help("Emit LLVM IR file")
      .default_value(false)
      .implicit_value(true);
  argParser.add_argument("--emit-obj")
      .help("Emit object file")
      .default_value(false)
      .implicit_value(true);
  argParser.add_argument("--emit-asm")
      .help("Emit assembly file")
      .default_value(false)
      .implicit_value(true);
  argParser.add_argument("--stdlib-path")
      .help("Path to the standard library")
      .default_value(std::string(""));

  try {
    argParser.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::println("{}", err.what());
    std::println("{}", argParser.help().str());
    return 1;
  }

  std::string inputFile = argParser.get<std::string>("input-file");
  std::string outputFile = argParser.get<std::string>("-o");
  bool dumpAst = argParser.get<bool>("--dump-ast");
  bool dumpTokens = argParser.get<bool>("--dump-tokens");
  bool dumpIR = argParser.get<bool>("--dump-ir");
  bool emitLLVM = argParser.get<bool>("--emit-llvm");
  bool emitObj = argParser.get<bool>("--emit-obj");
  bool emitAsm = argParser.get<bool>("--emit-asm");
  std::string stdlibPath = argParser.get<std::string>("--stdlib-path");

  std::ifstream file(inputFile);
  if (!file.is_open()) {
    std::println("Error: Could not open file: {}", inputFile);
    return 1;
  }

  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
  file.close();

  std::println("C hat Compiler (chc)");
  std::println("Compiling: {}", inputFile);

  try {
    if (dumpTokens) {
      std::println("\n=== Tokens ===");
      c_hat::lexer::Lexer lexer(source);
      while (auto token = lexer.nextToken()) {
        std::println("  {}: {}", static_cast<int>(token->getType()),
                     token->getValue());
        if (token->getType() == c_hat::lexer::TokenType::EndOfFile) {
          break;
        }
      }
    }

    c_hat::parser::Parser parser(source);
    auto program = parser.parseProgram();

    if (dumpAst) {
      std::println("\n=== Abstract Syntax Tree ===");
      for (const auto &decl : program->declarations) {
        std::println("  - {}", decl->toString());
      }
    }

    // 语义分析
    c_hat::semantic::SemanticAnalyzer semanticAnalyzer(stdlibPath);
    semanticAnalyzer.analyze(std::move(program));

    std::println("\n✓ Parsing and semantic analysis successful!");

    // 重新解析程序用于代码生成
    c_hat::parser::Parser parser2(source);
    auto program2 = parser2.parseProgram();

    c_hat::llvm_codegen::LLVMCodeGenerator codeGen("c_hat_module");
    codeGen.generate(std::move(program2));

    if (!codeGen.verifyIR()) {
      std::println("\n✗ IR verification failed!");
      return 1;
    }

    if (dumpIR) {
      std::println("\n=== LLVM IR ===");
      codeGen.printIR();
    }

    // 确定输出文件名
    std::filesystem::path inputPath(inputFile);
    std::string baseName = inputPath.stem().string();

    // 确定输出文件名
    std::string objOutputFile = baseName + ".obj";
    std::string exeOutputFile =
        outputFile.empty() ? (baseName + ".exe") : outputFile;

    // 生成输出文件
    if (emitLLVM) {
      std::string llvmOutputFile =
          outputFile.empty() ? (baseName + ".ll") : outputFile;
      if (codeGen.writeIRToFile(llvmOutputFile)) {
        std::println("\n✓ LLVM IR written to: {}", llvmOutputFile);
      }
    }

    if (emitObj) {
      if (outputFile.empty()) {
        objOutputFile = baseName + ".obj";
      } else {
        objOutputFile = outputFile;
      }
      if (codeGen.emitObjectFile(objOutputFile)) {
        std::println("\n✓ Object file written to: {}", objOutputFile);
      }
    }

    if (emitAsm) {
      std::string asmOutputFile =
          outputFile.empty() ? (baseName + ".s") : outputFile;
      if (codeGen.emitAssemblyFile(asmOutputFile)) {
        std::println("\n✓ Assembly file written to: {}", asmOutputFile);
      }
    }

    // 默认：如果没有指定 --emit-llvm/--emit-obj/--emit-asm，则生成可执行文件
    if (!emitLLVM && !emitObj && !emitAsm) {
      // 1. 先生成目标文件
      if (codeGen.emitObjectFile(objOutputFile)) {
        std::println("\n✓ Object file written to: {}", objOutputFile);

        // 2. 准备链接器参数 - 注意：字符串必须保持在作用域内
        std::string outArg = std::format("/OUT:{}", exeOutputFile);

        std::vector<std::string> argsStr;
        argsStr.push_back("lld-link");
        argsStr.push_back("/SUBSYSTEM:CONSOLE");
        argsStr.push_back("/ENTRY:main");
        argsStr.push_back(objOutputFile);
        argsStr.push_back("ucrt.lib");
        argsStr.push_back("vcruntime.lib");
        argsStr.push_back("msvcrt.lib");
        argsStr.push_back("kernel32.lib");
        argsStr.push_back("user32.lib");
        argsStr.push_back(outArg);

        std::vector<const char *> args;
        for (const auto &s : argsStr) {
          args.push_back(s.c_str());
        }

        // 3. 使用 LLD 库进行链接
        std::println("\nLinking with LLD (library)...");

        // 定义驱动程序
        std::vector<lld::DriverDef> drivers = LLD_ALL_DRIVERS;

        // 调用 LLD main
        lld::Result result =
            lld::lldMain(llvm::ArrayRef<const char *>(args.data(), args.size()),
                         llvm::outs(), llvm::errs(), drivers);

        if (result.retCode == 0) {
          std::println("\n✓ Executable generated: {}", exeOutputFile);
        } else {
          std::println("\n✗ Linking failed with code: {}", result.retCode);
        }
      }
    }

  } catch (const std::exception &e) {
    std::println("\n✗ Error: {}", e.what());
    return 1;
  }

  return 0;
}
