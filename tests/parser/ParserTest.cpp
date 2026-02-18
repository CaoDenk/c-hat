#include "../src/parser/Parser.h"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

using namespace c_hat;

TEST_CASE("Parser: Basic type parsing", "[parser][types]") {
  SECTION("Integer types") {
    std::vector<std::string> intTypes = {"int",   "uint",   "long", "ulong",
                                         "short", "ushort", "byte", "sbyte"};
    for (const auto &type : intTypes) {
      std::string source = type + " x;";
      parser::Parser parser(source);
      auto program = parser.parseProgram();
      REQUIRE(program != nullptr);
      REQUIRE(program->declarations.size() == 1);
    }
  }

  SECTION("Floating point types") {
    std::vector<std::string> floatTypes = {"float", "double"};
    for (const auto &type : floatTypes) {
      std::string source = type + " x;";
      parser::Parser parser(source);
      auto program = parser.parseProgram();
      REQUIRE(program != nullptr);
      REQUIRE(program->declarations.size() == 1);
    }
  }

  SECTION("Other basic types") {
    std::vector<std::string> types = {"bool", "char", "void"};
    for (const auto &type : types) {
      std::string source = type + " x;";
      parser::Parser parser(source);
      auto program = parser.parseProgram();
      REQUIRE(program != nullptr);
      REQUIRE(program->declarations.size() == 1);
    }
  }
}

TEST_CASE("Parser: Complex type parsing", "[parser][types]") {
  SECTION("Pointer type") {
    std::string source = "int^ ptr;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Array type") {
    std::string source = "int[10] arr;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Slice type") {
    std::string source = "int[] slice;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }
}

TEST_CASE("Parser: Variable declarations", "[parser][variables]") {
  SECTION("Explicit type declaration") {
    std::string source = "int x;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Var declaration") {
    std::string source = "var x = 42;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Let declaration") {
    std::string source = "let pi = 3.14;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Late declaration") {
    std::string source = "late int x;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Const declaration") {
    std::string source = "const int x = 10;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }
}

TEST_CASE("Parser: Function declarations", "[parser][functions]") {
  SECTION("Simple function") {
    std::string source = "func add(int a, int b) -> int { return a + b; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Function with arrow body") {
    std::string source = "func add(int a, int b) -> int => a + b;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Void function") {
    std::string source = "func print_hello() { println(\"Hello\"); }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Function with no parameters") {
    std::string source = "func get_value() -> int => 42;";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }
}

TEST_CASE("Parser: Control flow statements", "[parser][control]") {
  SECTION("If-else statement") {
    std::string source = "func test() { if (true) { } else { } }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("While statement") {
    std::string source =
        "func test() { var i = 0; while (i < 10) { i = i + 1; } }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("For statement") {
    std::string source =
        "func test() { for (var i = 0; i < 10; i = i + 1) { } }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Break/continue") {
    std::string source = "func test() { while (true) { break; } }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Parser: Exception handling", "[parser][exception]") {
  SECTION("Try-catch statement") {
    std::string source = "func test() { try { } catch (int e) { } }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Catch all") {
    std::string source = "func test() { try { } catch (...) { } }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Parser: defer statement", "[parser][defer]") {
  SECTION("Simple defer") {
    std::string source = "func test() { defer println(\"cleanup\"); }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Multiple defer") {
    std::string source =
        "func test() { defer println(\"1\"); defer println(\"2\"); }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Parser: Array literals", "[parser][arrays]") {
  SECTION("Integer array literal") {
    std::string source = "var arr = [1, 2, 3, 4, 5];";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("String array literal") {
    std::string source = "var arr = [\"a\", \"b\", \"c\"];";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }

  SECTION("Empty array literal") {
    std::string source = "var arr = [];";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }
}
