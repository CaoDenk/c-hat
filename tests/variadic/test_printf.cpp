#include "../src/llvm/LLVMCodeGenerator.h"
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <iostream>
#include <string>

using namespace c_hat;

int main() {
  std::string source = R"(
    extern "C" func printf(byte^ fmt, ...) -> int;

    func main() -> int {
      printf("Hello, world!\n");
      return 0;
    }
  )";

  std::cout << "Testing printf call..." << std::endl;

  try {
    // 解析
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      std::cout << "Parser failed!" << std::endl;
      return 1;
    }
    std::cout << "Parser succeeded!" << std::endl;

    // 语义分析
    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
    if (analyzer.hasError()) {
      std::cout << "Semantic analysis failed!" << std::endl;
      return 1;
    }
    std::cout << "Semantic analysis succeeded!" << std::endl;

    // 代码生成
    llvm_codegen::LLVMCodeGenerator generator("test_printf");
    generator.generate(std::move(program));
    std::cout << "Code generation succeeded!" << std::endl;

    // 打印 LLVM IR
    generator.printIR();

  } catch (const std::exception &e) {
    std::cout << "Exception: " << e.what() << std::endl;
  } catch (...) {
    std::cout << "Unknown exception!" << std::endl;
  }

  return 0;
}