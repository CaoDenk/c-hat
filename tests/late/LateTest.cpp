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

    semantic::SemanticAnalyzer analyzer("", false);
    analyzer.analyze(*program);
    return !analyzer.hasError();
  } catch (...) {
    return false;
  }
}

TEST_CASE("Late: Basic late declaration", "[late][basic]") {
  SECTION("Late with explicit type") {
    REQUIRE(analyzeSource("func test() { late int x; }") == true);
  }

  SECTION("Late with var is invalid") {
    REQUIRE(analyzeSource("func test() { late var x; }") == false);
  }

  SECTION("Late with class type") {
    REQUIRE(analyzeSource("class Person { } func test() { late Person p; }") == true);
  }

  SECTION("Late at global scope is invalid") {
    REQUIRE(analyzeSource("late int x;") == false);
  }

  SECTION("Late with initializer is invalid") {
    REQUIRE(analyzeSource("func test() { late int x = 1; }") == false);
  }

  SECTION("Non-late local without initializer is invalid") {
    REQUIRE(analyzeSource("func test() { int x; }") == false);
  }

  SECTION("Non-late var without initializer is invalid") {
    REQUIRE(analyzeSource("func test() { var x; }") == false);
  }
}

TEST_CASE("Late: let with explicit type", "[late][let]") {
  SECTION("late let with explicit type is valid") {
    REQUIRE(analyzeSource("func test() { late let int x; }") == true);
  }
}

TEST_CASE("Late: arrays require whole-value initialization", "[late][array]") {
  SECTION("Whole array assignment initializes") {
    REQUIRE(analyzeSource(
        "func test() -> int { late int[5] a; a = [1, 2, 3, 4, 5]; return a[0]; }") == true);
  }

  SECTION("Element writes do not initialize whole array") {
    REQUIRE(analyzeSource(
        "func test() -> int { late int[5] a; a[0] = 1; return a[0]; }") == false);
  }

  SECTION("Repeat initializer assigns whole array") {
    REQUIRE(analyzeSource(
        "func test() -> int { int[5] a = [0; 5]; return a[0]; }") == true);
  }
}

TEST_CASE("Late: Late in if-else", "[late][if_else]") {
  SECTION("Late initialized in if-else both branches") {
    REQUIRE(analyzeSource(
        "func test(bool cond) -> int { late int x; if (cond) { x = 10; } else { x = 20; } return x; }") == true);
  }

  SECTION("Late with else return") {
    REQUIRE(analyzeSource(
        "func test(bool cond) -> int { late int x; if (cond) { x = 10; } else { return 0; } return x; }") == true);
  }
}

TEST_CASE("Late: Late with try-catch", "[late][try_catch]") {
  SECTION("Late initialized in try block, used after") {
    REQUIRE(analyzeSource(
        "func test() -> int { late int x; try { x = 10; } catch (...) { } return x; }") == true);
  }

  SECTION("Late initialized in both try and catch") {
    REQUIRE(analyzeSource(
        "func test() -> int { late int x; try { x = 10; } catch (...) { x = 20; } return x; }") == true);
  }
}

TEST_CASE("Late: Late with class object", "[late][class]") {
  SECTION("Late class object declaration") {
    REQUIRE(analyzeSource(
        "class Person { public int age; } func test() { late Person p; }") == true);
  }
}

TEST_CASE("Late: Late with multiple variables", "[late][multiple]") {
  SECTION("Multiple late variables") {
    REQUIRE(analyzeSource(
        "func test(bool cond) -> int { late int a; late int b; if (cond) { a = 1; b = 2; } else { a = 3; b = 4; } return a + b; }") == true);
  }
}

TEST_CASE("Late: Late type mismatch errors", "[late][errors]") {
  SECTION("Late with incompatible type assignment") {
    REQUIRE(analyzeSource(
        "func test() -> int { late int x; x = 3.14; return x; }") == false);
  }
}
