#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

#include <iostream>

bool analyzeSource(const std::string &source) {
  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      std::cout << "Parser failed for: " << source << std::endl;
      return false;
    }

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
    bool result = !analyzer.hasError();
    if (!result) {
      std::cout << "Semantic analysis failed for: " << source << std::endl;
    }
    return result;
  } catch (const std::exception &e) {
    std::cout << "Exception for: " << source << " - " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cout << "Unknown exception for: " << source << std::endl;
    return false;
  }
}

TEST_CASE("Variadic: Basic variadic function", "[variadic]") {
  SECTION("Variadic function declaration") {
    REQUIRE(analyzeSource("func print(int^ fmt, ...) { }") == true);
  }

  SECTION("Variadic function with type") {
    REQUIRE(analyzeSource("extern \"C\" func printf(byte^ fmt, ...) -> int;") ==
            true);
  }
}

TEST_CASE("Variadic: Error cases", "[variadic][errors]") {
  SECTION("Variadic not at end - should fail") {
    REQUIRE(analyzeSource("func test(int a, ..., int b) { }") == false);
  }

  SECTION("Multiple variadic - should fail") {
    REQUIRE(analyzeSource("func test(int a, ..., ...) { }") == false);
  }
}
