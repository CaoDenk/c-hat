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

  auto filePath1 = basePath;
  filePath1 += ".ch";

  auto filePath2 = basePath / "mod.ch";

  if (fs::exists(filePath1)) {
    return filePath1;
  }
  if (fs::exists(filePath2)) {
    return filePath2;
  }

  return filePath1;
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

    // 生成输出文件
    if (emitLLVM) {
      std::string llvmOutputFile =
          outputFile.empty() ? (baseName + ".ll") : outputFile;
      if (codeGen.writeIRToFile(llvmOutputFile)) {
        std::println("\n✓ LLVM IR written to: {}", llvmOutputFile);
      }
    }

    if (emitObj) {
      std::string objOutputFile =
          outputFile.empty() ? (baseName + ".obj") : outputFile;
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

  } catch (const std::exception &e) {
    std::println("\n✗ Error: {}", e.what());
    return 1;
  }

  return 0;
}
