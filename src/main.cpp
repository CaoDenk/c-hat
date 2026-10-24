#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"
#include "llvm/LLVMCodeGenerator.h"
#include "Diagnostics.h"
#include <argparse/argparse.hpp>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef USE_LLD
#include <lld/Common/Driver.h>
#include <lld/Common/ErrorHandler.h>

LLD_HAS_DRIVER(coff)
#endif

namespace fs = std::filesystem;

namespace {

#ifdef _WIN32
constexpr const char *kObjectExtension = ".obj";
constexpr const char *kExecutableExtension = ".exe";
#else
constexpr const char *kObjectExtension = ".o";
constexpr const char *kExecutableExtension = "";
#endif

// Quote one argument for the platform shell.  Windows keeps the historical
// raw-path behaviour (the linker command is assembled by the caller); POSIX
// uses single quotes so paths with spaces are safe.
std::string quoteShellArg(const std::string &value) {
#ifdef _WIN32
  if (value.find(' ') == std::string::npos) {
    return value;
  }
  return "\"" + value + "\"";
#else
  std::string out = "'";
  for (char c : value) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out += c;
    }
  }
  out += "'";
  return out;
#endif
}

} // namespace


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

extern "C" {
int c_hat_printf(const char *format, ...);
}

int main(int argc, char *argv[]) {
  argparse::ArgumentParser argParser("C hat Compiler (chc)");

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
  argParser.add_argument("--no-verify")
      .help("Skip LLVM IR verification (for debugging malformed IR)")
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
  argParser.add_argument("--run")
      .help("Run the program directly using JIT (no linking required)")
      .default_value(false)
      .implicit_value(true);
  argParser.add_argument("--stdlib-path")
      .help("Path to the standard library")
      .default_value(std::string(""));
  argParser.add_argument("-M", "--module-path")
      .help("Additional module search paths (can be specified multiple times)")
      .default_value(std::vector<std::string>())
      .append();
  argParser.add_argument("-l", "--library")
      .help("Libraries to link")
      .default_value(std::vector<std::string>())
      .append();
  argParser.add_argument("--c-lib-path")
      .help("Path to the C standard library (Windows: extra /LIBPATH; Linux: extra -L)")
      .default_value(std::string(""));
  argParser.add_argument("--c-lib-file")
      .help("C standard library file to link")
      .default_value(std::string(""));
  argParser.add_argument("--linker")
      .help("Linker driver to use on non-Windows hosts (default: $CXX or c++)")
      .default_value(std::string(""));
  argParser.add_argument("--verbose")
      .help("Print detailed progress information")
      .default_value(false)
      .implicit_value(true);

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
  bool noVerify = argParser.get<bool>("--no-verify");
  bool emitLLVM = argParser.get<bool>("--emit-llvm");
  bool emitObj = argParser.get<bool>("--emit-obj");
  bool emitAsm = argParser.get<bool>("--emit-asm");
  bool runJIT = argParser.get<bool>("--run");
  std::string stdlibPath = argParser.get<std::string>("--stdlib-path");
  std::vector<std::string> modulePaths =
      argParser.get<std::vector<std::string>>("--module-path");
  std::vector<std::string> libraries =
      argParser.get<std::vector<std::string>>("--library");
  std::string cLibPath = argParser.get<std::string>("--c-lib-path");
  std::string cLibFile = argParser.get<std::string>("--c-lib-file");
  std::string linker = argParser.get<std::string>("--linker");
  bool verbose = argParser.get<bool>("--verbose");

  c_hat::debugLogsEnabled() = verbose;

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

    if (!program) {
      std::println("Error: Failed to parse program");
      return 1;
    }

    if (verbose) {
      std::println("Parsed {} top-level declaration(s).",
                   program->declarations.size());
    }

    if (dumpAst) {
      std::println("\n=== Abstract Syntax Tree ===");
      for (const auto &decl : program->declarations) {
        std::println("  - {}", decl->toString());
      }
    }

    std::vector<std::string> allModulePaths;
    if (!stdlibPath.empty()) {
      allModulePaths.push_back(stdlibPath);
    }
    for (const auto &path : modulePaths) {
      allModulePaths.push_back(path);
    }

    c_hat::semantic::SemanticAnalyzer semanticAnalyzer(allModulePaths);
    if (verbose) {
      std::println("Running semantic analysis...");
    }
    semanticAnalyzer.analyze(*program);

    if (semanticAnalyzer.hasError()) {
      std::println("\n✗ Semantic analysis failed!");
      return 1;
    }

    std::println("✓ Parsing and semantic analysis successful!");

    if (verbose) {
      std::println("Generating LLVM IR...");
    }
    c_hat::llvm_codegen::LLVMCodeGenerator codeGen("c_hat_module");
    codeGen.generate(std::move(program));
    if (verbose) {
      std::println("LLVM IR generated.");
    }

    if (!noVerify && !codeGen.verifyIR()) {
      std::println("\n✗ IR verification failed!");
      return 1;
    }

    if (dumpIR) {
      std::println("\n=== LLVM IR ===");
      codeGen.printIR();
    }

    fs::path inputPath(inputFile);
    std::string baseName = inputPath.stem().string();

    if (runJIT) {
      std::println("\n=== Running with JIT ===");

      codeGen.addExternalSymbol("printf", (void *)&printf);

      int result = codeGen.runJIT("main");
      std::println("\n✓ Program exited with code: {}", result);
      return result;
    }

    std::string objOutputFile = baseName + kObjectExtension;
    std::string exeOutputFile =
        outputFile.empty() ? (baseName + kExecutableExtension) : outputFile;

    if (emitLLVM) {
      std::string llvmOutputFile =
          outputFile.empty() ? (baseName + ".ll") : outputFile;
      if (codeGen.writeIRToFile(llvmOutputFile)) {
        std::println("\n✓ LLVM IR written to: {}", llvmOutputFile);
      }
    }

    if (emitObj) {
      if (outputFile.empty()) {
        objOutputFile = baseName + kObjectExtension;
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

    if (!emitLLVM && !emitObj && !emitAsm) {
      if (codeGen.emitObjectFile(objOutputFile)) {
        std::println("\n✓ Object file written to: {}", objOutputFile);

#ifdef _WIN32
        std::string outArg = std::format("/OUT:{}", exeOutputFile);

        std::vector<std::string> argsStr;
        argsStr.push_back("lld-link");
        argsStr.push_back(outArg);
        argsStr.push_back("/DEFAULTLIB:libcmt");
        argsStr.push_back("/DEFAULTLIB:oldnames");

        std::vector<std::string> libPaths = {
            "C:/Program Files/Microsoft Visual "
            "Studio/2022/Community/VC/Tools/MSVC/14.44.35207/lib/x64",
            "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64",
            "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"};

        if (!cLibPath.empty()) {
          libPaths.push_back(cLibPath);
        }

        for (const auto &path : libPaths) {
          argsStr.push_back("/LIBPATH:" + path);
        }

        argsStr.push_back(objOutputFile);

        if (!cLibFile.empty()) {
          argsStr.push_back(cLibFile);
        }

        for (const auto &lib : libraries) {
          argsStr.push_back(lib);
        }

        std::println("\nLinking with LLD...");

#ifdef USE_LLD
        std::vector<const char *> args;
        for (const auto &arg : argsStr) {
          args.push_back(arg.c_str());
        }

        std::vector<lld::DriverDef> drivers = {
            {lld::WinLink, &lld::coff::link}};

        lld::Result result =
            lld::lldMain(llvm::ArrayRef<const char *>(args.data(), args.size()),
                         llvm::outs(), llvm::errs(), drivers);

        if (result.retCode == 0) {
          std::println("\n✓ Executable generated: {}", exeOutputFile);
        } else {
          std::println("\n✗ Linking failed with code: {}", result.retCode);
        }
#else
        std::string linkCommand = "link.exe";
        linkCommand += " /OUT:" + exeOutputFile;
        linkCommand += " /DEFAULTLIB:libcmt";
        linkCommand += " /DEFAULTLIB:oldnames";

        for (const auto &path : libPaths) {
          linkCommand += " /LIBPATH:" + path;
        }

        linkCommand += " " + objOutputFile;

        if (!cLibFile.empty()) {
          linkCommand += " " + cLibFile;
        }

        for (const auto &lib : libraries) {
          linkCommand += " " + lib;
        }

        int linkResult = std::system(linkCommand.c_str());

        if (linkResult == 0) {
          std::println("\n✓ Executable generated: {}", exeOutputFile);
        } else {
          std::println("\n✗ Linking failed with code: {}", linkResult);
        }
#endif
#else
        // POSIX: the object file is a native ELF/Mach-O object.  Use the
        // configured C++ driver (or $CXX) so startup files and libc are linked
        // exactly the way the host expects.  This also keeps cross-compiling
        // possible by pointing --linker at a target-specific driver.
        std::string linkDriver = linker;
        if (linkDriver.empty()) {
          if (const char *envCxx = std::getenv("CXX");
              envCxx != nullptr && *envCxx != '\0') {
            linkDriver = envCxx;
          }
        }
        if (linkDriver.empty()) {
          linkDriver = "c++";
        }

        std::vector<std::string> linkArgs;
        linkArgs.push_back(quoteShellArg(linkDriver));
        // The object emitter currently produces non-PIC code, while most
        // Linux distributions default to PIE.  Ask the driver for a
        // non-PIE executable so absolute relocations stay valid.
#ifdef __APPLE__
        linkArgs.push_back("-Wl,-no_pie");
#else
        linkArgs.push_back("-no-pie");
#endif
        linkArgs.push_back("-o");
        linkArgs.push_back(quoteShellArg(exeOutputFile));
        linkArgs.push_back(quoteShellArg(objOutputFile));

        if (!cLibPath.empty()) {
          linkArgs.push_back(quoteShellArg("-L" + cLibPath));
        }
        if (!cLibFile.empty()) {
          linkArgs.push_back(quoteShellArg(cLibFile));
        }
        for (const auto &lib : libraries) {
          if (lib.rfind("-l", 0) == 0 || lib.rfind("-L", 0) == 0 ||
              lib.find('/') != std::string::npos ||
              lib.find(".a") != std::string::npos ||
              lib.find(".so") != std::string::npos) {
            linkArgs.push_back(quoteShellArg(lib));
          } else {
            linkArgs.push_back(quoteShellArg("-l" + lib));
          }
        }
        linkArgs.push_back("-lm");

        std::string linkCommand;
        for (const auto &arg : linkArgs) {
          linkCommand += " " + arg;
        }

        std::println("\nLinking with {}...", linkDriver);
        if (verbose) {
          std::println("{}", linkCommand);
        }

        int linkResult = std::system(linkCommand.c_str());
        if (linkResult == 0) {
          std::println("\n✓ Executable generated: {}", exeOutputFile);
        } else {
          std::println("\n✗ Linking failed with code: {}", linkResult);
        }
#endif
      }
    }

  } catch (const std::exception &e) {
    std::println("\n✗ Error: {}", e.what());
    return 1;
  }

  return 0;
}
