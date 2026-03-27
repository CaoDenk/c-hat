#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    std::string sourceWithMain = source + "\nfunc main() { }\n";
    parser::Parser parser(sourceWithMain);
    auto program = parser.parseProgram();
    if (!program)
      return false;

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
    return !analyzer.hasError();
  } catch (...) {
    return false;
  }
}

TEST_CASE("Late: Basic late declaration", "[late][basic]") {
  SECTION("Late with explicit type") {
    REQUIRE(analyzeSource("late int x;") == true);
  }

  SECTION("Late with var") {
    REQUIRE(analyzeSource("late var x;") == true);
  }

  SECTION("Late with class type") {
    REQUIRE(analyzeSource("class Person { } late Person p;") == true);
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
