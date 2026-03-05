#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"
#include <iostream>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      std::cout << "Parser failed" << std::endl;
      return false;
    }

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
    bool result = !analyzer.hasError();
    std::cout << "Semantic analysis result: " << (result ? "success" : "failed")
              << std::endl;
    return result;
  } catch (const std::exception &e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cout << "Unknown exception" << std::endl;
    return false;
  }
}

int main() {
  std::cout << "Testing tuple literal: var t = (1, 2);" << std::endl;
  bool result1 = analyzeSource("func test() { var t = (1, 2); }");
  std::cout << "Result: " << (result1 ? "PASS" : "FAIL") << std::endl
            << std::endl;

  std::cout << "Testing tuple destructuring: var (x, y) = (10, 20);"
            << std::endl;
  bool result2 = analyzeSource("func test() { var (x, y) = (10, 20); }");
  std::cout << "Result: " << (result2 ? "PASS" : "FAIL") << std::endl
            << std::endl;

  std::cout << "Testing tuple as return type: return (10, 20);" << std::endl;
  bool result3 =
      analyzeSource("func get_point() -> (int, int) { return (10, 20); }");
  std::cout << "Result: " << (result3 ? "PASS" : "FAIL") << std::endl
            << std::endl;

  return 0;
}
