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

TEST_CASE("Overload: Function overloading by parameter type",
          "[overload][params]") {
  SECTION("Overload with int and float") {
    REQUIRE(analyzeSource("func print(int x) { } "
                          "func print(float x) { }") == true);
  }

  SECTION("Overload with int and string") {
    REQUIRE(analyzeSource("func process(int x) { } "
                          "func process(string s) { }") == true);
  }

  SECTION("Overload with different param counts") {
    REQUIRE(analyzeSource("func add(int a, int b) -> int { return a + b; } "
                          "func add(int a) -> int { return a; }") == true);
  }
}

TEST_CASE("Overload: Constructor overloading", "[overload][constructor]") {
  SECTION("Class with multiple constructors") {
    REQUIRE(analyzeSource("class Point { "
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
    REQUIRE(analyzeSource("func foo(int x) { } "
                          "func foo(int x) { }") == false);
  }

  SECTION("Same signature different return type") {
    REQUIRE(analyzeSource("func foo(int x) -> int { return x; } "
                          "func foo(int x) -> float { return 0.0; }") == false);
  }
}

TEST_CASE("Operator Overload: Parse binary operators", "[operator][parse]") {
  SECTION("Parse operator+") {
    REQUIRE(analyzeSource("class Vector { "
                          "  int x; int y; "
                          "  operator+(Vector rhs) -> Vector { return self; } "
                          "}") == true);
  }

  SECTION("Parse operator-") {
    REQUIRE(analyzeSource("class Vector { "
                          "  operator-(Vector rhs) -> Vector { return self; } "
                          "}") == true);
  }

  SECTION("Parse operator*") {
    REQUIRE(analyzeSource("class Vector { "
                          "  operator*(int scalar) -> Vector { return self; } "
                          "}") == true);
  }

  SECTION("Parse operator[]") {
    REQUIRE(analyzeSource("class Array { "
                          "  operator[](int index) -> int { return 0; } "
                          "}") == true);
  }

  SECTION("Parse operator()") {
    REQUIRE(analyzeSource("class Functor { "
                          "  operator()(int x) -> int { return x; } "
                          "}") == true);
  }

  SECTION("Parse operator==") {
    REQUIRE(analyzeSource("class Point { "
                          "  operator==(Point other) -> bool { return true; } "
                          "}") == true);
  }

  SECTION("Parse operator!=") {
    REQUIRE(analyzeSource("class Point { "
                          "  operator!=(Point other) -> bool { return false; } "
                          "}") == true);
  }

  SECTION("Parse operator<") {
    REQUIRE(analyzeSource("class Value { "
                          "  operator<(Value other) -> bool { return true; } "
                          "}") == true);
  }

  SECTION("Parse operator+=") {
    REQUIRE(
        analyzeSource("class Counter { "
                      "  int value; "
                      "  operator+=(int x) { self.value = self.value + x; } "
                      "}") == true);
  }
}

TEST_CASE("Operator Overload: Parse bitwise and logical operators",
          "[operator][bitwise]") {
  SECTION("Parse operator&") {
    REQUIRE(analyzeSource("class Flags { "
                          "  operator&(Flags other) -> Flags { return self; } "
                          "}") == true);
  }

  SECTION("Parse operator|") {
    REQUIRE(analyzeSource("class Flags { "
                          "  operator|(Flags other) -> Flags { return self; } "
                          "}") == true);
  }

  SECTION("Parse operator<<") {
    REQUIRE(analyzeSource("class Stream { "
                          "  operator<<(int x) -> Stream { return self; } "
                          "}") == true);
  }

  SECTION("Parse operator>>") {
    REQUIRE(analyzeSource("class Stream { "
                          "  operator>>(int x) -> Stream { return self; } "
                          "}") == true);
  }

  SECTION("Parse operator&&") {
    REQUIRE(
        analyzeSource("class BoolLike { "
                      "  operator&&(BoolLike other) -> bool { return true; } "
                      "}") == true);
  }

  SECTION("Parse operator||") {
    REQUIRE(
        analyzeSource("class BoolLike { "
                      "  operator||(BoolLike other) -> bool { return true; } "
                      "}") == true);
  }
}

TEST_CASE("Operator Overload: Parse unary operators", "[operator][unary]") {
  SECTION("Parse operator!") {
    REQUIRE(analyzeSource("class BoolLike { "
                          "  operator!() -> bool { return false; } "
                          "}") == true);
  }

  SECTION("Parse operator~") {
    REQUIRE(analyzeSource("class Bits { "
                          "  operator~() -> Bits { return self; } "
                          "}") == true);
  }
}

TEST_CASE("Function Pointer: Parse function pointer type", "[funcptr][parse]") {
  SECTION("Parse simple function pointer type in parameter") {
    REQUIRE(analyzeSource(
                "func apply(int a, int b, func(int, int) -> int op) -> int { "
                "  return a; "
                "}") == true);
  }

  SECTION("Parse function pointer with void return in parameter") {
    REQUIRE(analyzeSource("func setCallback(func(int) callback) { }") == true);
  }

  SECTION("Parse function pointer with multiple params") {
    REQUIRE(
        analyzeSource(
            "func process(func(int, float, string) -> bool validator) { }") ==
        true);
  }
}

TEST_CASE("Function Pointer: Function pointer as parameter",
          "[funcptr][param]") {
  SECTION("Function pointer parameter") {
    REQUIRE(analyzeSource(
                "func apply(int a, int b, func(int, int) -> int op) -> int { "
                "  return a; "
                "}") == true);
  }

  SECTION("Function pointer callback parameter") {
    REQUIRE(analyzeSource(
                "func processData(string data, func(string) callback) { }") ==
            true);
  }
}

TEST_CASE("Function Pointer: Function pointer as return type",
          "[funcptr][return]") {
  SECTION("Return function pointer") {
    REQUIRE(analyzeSource("func getHandler() -> func(int) -> int { "
                          "}") == true);
  }
}

TEST_CASE("Function Pointer: Type alias for function pointer",
          "[funcptr][alias]") {
  SECTION("Using type alias for function pointer") {
    REQUIRE(analyzeSource("using BinaryOp = func(int, int) -> int; "
                          "func compute(int a, int b, BinaryOp op) -> int { "
                          "  return a; "
                          "}") == true);
  }
}
