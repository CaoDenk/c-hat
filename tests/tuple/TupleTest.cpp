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
    return !analyzer.hasError();
  } catch (...) {
    return false;
  }
}

TEST_CASE("Tuple: Basic tuple literal", "[tuple][literal]") {
  SECTION("Two-element tuple") {
    REQUIRE(analyzeSource("func test() { var t = (1, \"hello\"); }") == true);
  }

  SECTION("Three-element tuple") {
    REQUIRE(analyzeSource("func test() { var t = (3.14, true, 42); }") == true);
  }
}

TEST_CASE("Tuple: var vs let", "[tuple][variable]") {
  SECTION("var can reassign") {
    REQUIRE(analyzeSource("func test() { var t = (1, 2); t = (3, 4); }") ==
            true);
  }

  SECTION("let cannot reassign") {
    REQUIRE(analyzeSource("func test() { let t = (1, 2); t = (3, 4); }") ==
            false);
  }
}

TEST_CASE("Tuple: as parameter", "[tuple][parameter]") {
  SECTION("Tuple as function parameter") {
    REQUIRE(analyzeSource("func print_point((int, int) point) { }") == true);
  }
}

TEST_CASE("Tuple: as return type", "[tuple][return]") {
  SECTION("Tuple as function return type") {
    REQUIRE(analyzeSource(
                "func get_point() -> (int, int) { return (10, 20); }") == true);
  }
}

TEST_CASE("Tuple: destructuring", "[tuple][destructuring]") {
  SECTION("Declare and destruct") {
    REQUIRE(analyzeSource("func test() { var (x, y) = (10, 20); }") == true);
  }
}
