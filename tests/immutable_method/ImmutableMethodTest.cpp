#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      std::cerr << "Parse failed" << std::endl;
      return false;
    }

    semantic::SemanticAnalyzer analyzer("", false);
    analyzer.analyze(*program);
    if (analyzer.hasError()) {
      std::cerr << "Semantic errors found" << std::endl;
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cerr << "Unknown exception" << std::endl;
    return false;
  }
}

TEST_CASE("Immutable: Function suffix ! for const method", "[immutable][suffix]") {
  SECTION("Function with ! suffix returns int") {
    REQUIRE(analyzeSource(
        "class Counter { private int count; public func getCount()! -> int { return count; } }") == true);
  }

  SECTION("Function with ! suffix returns string") {
    REQUIRE(analyzeSource(
        "class Person { private string name; public func getName()! -> string { return name; } }") == true);
  }

  SECTION("Void function with ! suffix") {
    REQUIRE(analyzeSource(
        "class Logger { public func log()! { } }") == true);
  }
}

TEST_CASE("Immutable: self! parameter", "[immutable][self]") {
  SECTION("Function with self! parameter") {
    REQUIRE(analyzeSource(
        "class Point { int x; int y; public func length(self!) -> float { return 0.0; } }") == true);
  }

  SECTION("Function with self! and parameters") {
    REQUIRE(analyzeSource(
        "class Math { public func add(self!, int a, int b) -> int { return a + b; } }") == true);
  }
}

TEST_CASE("Immutable: Implicit self (no self keyword)", "[immutable][implicit]") {
  SECTION("Method without self keyword") {
    REQUIRE(analyzeSource(
        "class Calculator { public int add(int a, int b) { return a + b; } }") == true);
  }

  SECTION("Method without self keyword accessing member") {
    REQUIRE(analyzeSource(
        "class Counter { private int count; public int get() { return count; } }") == true);
  }
}

TEST_CASE("Immutable: Mixed method styles", "[immutable][mixed]") {
  SECTION("Class with both implicit and explicit self") {
    REQUIRE(analyzeSource(
        "class Vector { "
        "  int x; int y; "
        "  public func reset() { x = 0; y = 0; } "
        "  public func length()! -> float { return 0.0; } "
        "}") == true);
  }
}


TEST_CASE("Immutable: Operator overload requires self", "[immutable][operator]") {
  SECTION("Operator+ with self!") {
    REQUIRE(analyzeSource(
        "class Point { "
        "  int x; int y; "
        "  public func operator+(self!, Point other) -> Point { return Point{0, 0}; } "
        "}") == true);
  }

  SECTION("Operator== with self!") {
    REQUIRE(analyzeSource(
        "class Point { "
        "  int x; int y; "
        "  public func operator==(self!, Point other) -> bool { return true; } "
        "}") == true);
  }
}
