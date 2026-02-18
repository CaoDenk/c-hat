#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>


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

TEST_CASE("Semantic: Basic type checking", "[semantic][types]") {
  SECTION("Integer types") {
    REQUIRE(analyzeSource("int x;") == true);
    REQUIRE(analyzeSource("uint x;") == true);
    REQUIRE(analyzeSource("long x;") == true);
    REQUIRE(analyzeSource("ulong x;") == true);
  }

  SECTION("Floating point types") {
    REQUIRE(analyzeSource("float x;") == true);
    REQUIRE(analyzeSource("double x;") == true);
  }

  SECTION("Bool and char") {
    REQUIRE(analyzeSource("bool x;") == true);
    REQUIRE(analyzeSource("char x;") == true);
  }
}

TEST_CASE("Semantic: Variable declarations", "[semantic][variables]") {
  SECTION("Explicit type declaration") {
    REQUIRE(analyzeSource("int x;") == true);
  }

  SECTION("Var declaration with initializer") {
    REQUIRE(analyzeSource("var x = 42;") == true);
  }

  SECTION("Let declaration with initializer") {
    REQUIRE(analyzeSource("let pi = 3.14;") == true);
  }

  SECTION("Late variable") { REQUIRE(analyzeSource("late int x;") == true); }

  SECTION("Const variable") {
    REQUIRE(analyzeSource("const int x = 10;") == true);
  }
}

TEST_CASE("Semantic: Function declarations", "[semantic][functions]") {
  SECTION("Simple function") {
    REQUIRE(analyzeSource("func add(int a, int b) -> int { return a + b; }") ==
            true);
  }

  SECTION("Function with arrow body") {
    REQUIRE(analyzeSource("func add(int a, int b) -> int => a + b;") == true);
  }

  SECTION("Void function") {
    REQUIRE(analyzeSource("func print_hello() { }") == true);
  }

  SECTION("Function with return") {
    REQUIRE(analyzeSource("func get_value() -> int => 42;") == true);
  }
}

TEST_CASE("Semantic: Class declarations", "[semantic][classes]") {
  SECTION("Empty class") { REQUIRE(analyzeSource("class Person {}") == true); }
}

TEST_CASE("Semantic: Array literals", "[semantic][arrays]") {
  SECTION("Integer array literal") {
    REQUIRE(analyzeSource("var arr = [1, 2, 3, 4, 5];") == true);
  }

  SECTION("String array literal") {
    REQUIRE(analyzeSource("var arr = [\"a\", \"b\", \"c\"];") == true);
  }

  SECTION("Empty array literal") {
    REQUIRE(analyzeSource("var arr = [];") == true);
  }
}

TEST_CASE("Semantic: Late variables", "[semantic][late]") {
  SECTION("Late variable with type") {
    REQUIRE(analyzeSource("late int x;") == true);
  }

  SECTION("Late variable without type") {
    REQUIRE(analyzeSource("late var x;") == true);
  }
}

TEST_CASE("Semantic: Control flow", "[semantic][control]") {
  SECTION("If-else") {
    REQUIRE(analyzeSource("func test() { if (true) { } else { } }") == true);
  }

  SECTION("While") {
    REQUIRE(analyzeSource(
                "func test() { var i = 0; while (i < 10) { i = i + 1; } }") ==
            true);
  }

  SECTION("For") {
    REQUIRE(analyzeSource(
                "func test() { for (var i = 0; i < 10; i = i + 1) { } }") ==
            true);
  }
}

TEST_CASE("Semantic: Expression type checking", "[semantic][expressions]") {
  SECTION("Arithmetic expressions") {
    REQUIRE(analyzeSource("func test() -> int { return 1 + 2 * 3; }") == true);
  }

  SECTION("Comparison expressions") {
    REQUIRE(analyzeSource("func test() -> bool { return 1 < 2 && 3 > 4; }") ==
            true);
  }

  SECTION("Function call") {
    REQUIRE(analyzeSource("func foo(int x) -> int { return x; } func test() -> "
                          "int { return foo(42); }") == true);
  }
}
