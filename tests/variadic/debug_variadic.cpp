#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <iostream>
#include <string>

using namespace c_hat;

int main() {
  std::string source = "func print(fmt: *int, ...) { }";
  std::cout << "Testing: " << source << std::endl;

  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      std::cout << "Parser failed!" << std::endl;
      return 1;
    }
    std::cout << "Parser succeeded!" << std::endl;

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
    if (analyzer.hasError()) {
      std::cout << "Semantic analysis failed!" << std::endl;
    } else {
      std::cout << "Semantic analysis succeeded!" << std::endl;
    }
  } catch (const std::exception &e) {
    std::cout << "Exception: " << e.what() << std::endl;
  } catch (...) {
    std::cout << "Unknown exception!" << std::endl;
  }

  return 0;
}
