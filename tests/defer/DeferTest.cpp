#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program)
      return false;

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(std::move(program));
    return true;
  } catch (...) {
    return false;
  }
}

TEST_CASE("Defer: Basic defer", "[defer][basic]") {
  SECTION("Simple defer") {
    REQUIRE(analyzeSource("func test() { defer println(\"cleanup\"); }") ==
            true);
  }

  SECTION("Defer with expression") {
    REQUIRE(analyzeSource("func test() { var x = 1; defer x = x + 1; }") ==
            true);
  }
}

TEST_CASE("Defer: Multiple defer", "[defer][multiple]") {
  SECTION("Two defer statements") {
    REQUIRE(
        analyzeSource(
            "func test() { defer println(\"1\"); defer println(\"2\"); }") ==
        true);
  }
}

TEST_CASE("Defer: Defer in function", "[defer][function]") {
  SECTION("Defer with return") {
    REQUIRE(analyzeSource(
                "func test() -> int { defer cleanup(); return 42; }") == true);
  }
}
