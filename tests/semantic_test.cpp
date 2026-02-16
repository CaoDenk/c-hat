#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <format>
#include <iostream>
#include <print>
#include <string>
#include <vector>

struct TestCase {
  std::string name;
  std::string source;
};

void runTestCase(const TestCase &testCase) {
  std::println("\n=== Testing: {} ===", testCase.name);
  std::println("Source: {}", testCase.source);

  try {
    c_hat::parser::Parser parser(testCase.source);
    auto program = parser.parseProgram();
    std::println("✓ Parsed successfully!");

    c_hat::semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(std::move(program));
    std::println("✓ Semantic analysis completed!");
  } catch (const std::exception &e) {
    std::println("✗ Error: {}", e.what());
  }
}

int main() {
  std::vector<TestCase> testCases = {
      {"Simple function", "func add(int a, int b) -> int { return a + b; }"},
      {"Function with arrow", "func add(int a, int b) -> int => a + b;"},
      {"Explicit variable", "int x;"},
      {"Var variable with initializer", "var x = 42;"},
      {"Let variable with initializer", "let pi = 3.14;"},
      {"Class declaration", "class Person { var name; var age; }"},
      {"Struct declaration", "struct Point { var x; var y; }"},
      {"Extension declaration", "extension int { var temp = 0; }"}};

  for (const auto &testCase : testCases) {
    runTestCase(testCase);
  }

  std::println("\n=== All tests completed ===");

  return 0;
}
