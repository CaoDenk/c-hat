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

TEST_CASE("Overload: Function overloading by parameter type", "[overload][params]") {
  SECTION("Overload with int and float") {
    REQUIRE(analyzeSource(
        "func print(int x) { } "
        "func print(float x) { }") == true);
  }

  SECTION("Overload with int and string") {
    REQUIRE(analyzeSource(
        "func process(int x) { } "
        "func process(string s) { }") == true);
  }

  SECTION("Overload with different param counts") {
    REQUIRE(analyzeSource(
        "func add(int a, int b) -> int { return a + b; } "
        "func add(int a) -> int { return a; }") == true);
  }
}

TEST_CASE("Overload: Constructor overloading", "[overload][constructor]") {
  SECTION("Class with multiple constructors") {
    REQUIRE(analyzeSource(
        "class Point { "
        "  int x; int y; "
        "  Point() { x = 0; y = 0; } "
        "  Point(int x, int y) { self.x = x; self.y = y; } "
        "}") == true);
  }
}

TEST_CASE("Overload: Method overloading", "[overload][method]") {
  SECTION("Class with overloaded methods") {
    REQUIRE(analyzeSource(
        "class Calculator { "
        "  public int add(int a, int b) { return a + b; } "
        "  public int add(int a, int b, int c) { return a + b + c; } "
        "}") == true);
  }
}

TEST_CASE("Overload: Same signature should error", "[overload][errors]") {
  SECTION("Duplicate function signature") {
    REQUIRE(analyzeSource(
        "func foo(int x) { } "
        "func foo(int x) { }") == false);
  }

  SECTION("Same signature different return type") {
    REQUIRE(analyzeSource(
        "func foo(int x) -> int { return x; } "
        "func foo(int x) -> float { return 0.0; }") == false);
  }
}
