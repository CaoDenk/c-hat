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
  SECTION("Declare and destruct two elements") {
    REQUIRE(analyzeSource("func test() { var (x, y) = (10, 20); }") == true);
  }

  SECTION("Declare and destruct three elements") {
    REQUIRE(analyzeSource(
                "func test() { var (a, b, c) = (10, 3.14, \"hello\"); }") ==
            true);
  }

  SECTION("Let tuple destructuring") {
    REQUIRE(analyzeSource("func test() { let (x, y) = (10, 20); }") == true);
  }
}

TEST_CASE("Tuple: const tuple", "[tuple][const]") {
  SECTION("const tuple declaration") {
    REQUIRE(analyzeSource("func test() { const (int, int) t = (10, 20); }") ==
            true);
  }
}

TEST_CASE("Tuple: Error path tests", "[tuple][errors]") {
  SECTION("Let tuple reassignment") {
    REQUIRE(analyzeSource("func test() { let t = (1, 2); t = (3, 4); }") ==
            false);
  }

  SECTION("Tuple destructuring with wrong element count (more)") {
    REQUIRE(analyzeSource("func test() { var (x, y) = (1, 2, 3); }") == false);
  }

  SECTION("Tuple destructuring with wrong element count (less)") {
    REQUIRE(analyzeSource("func test() { var (x, y, z) = (1, 2); }") == false);
  }
}
