#include "../src/parser/Parser.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

TEST_CASE("Class: Basic class declaration", "[class][basic]") {
  SECTION("Empty class") {
    std::string source = "class Person {}";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }
}

TEST_CASE("Class: Access modifiers", "[class][access]") {
  SECTION("Public class") {
    std::string source = "public class Person {}";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Private class") {
    std::string source = "private class Person {}";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Class: Class with members", "[class][members]") {
  SECTION("Class with member variables") {
    std::string source = "class Person { var name; var age; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Class with member functions") {
    std::string source = "class Person { func greet() { } }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Class: Member access modifiers", "[class][members][access]") {
  SECTION("Public member") {
    std::string source = "class Person { public var name; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Private member") {
    std::string source = "class Person { private var name; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Protected member") {
    std::string source = "class Person { protected var name; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Class: Multiple classes", "[class][multiple]") {
  SECTION("Two classes") {
    std::string source = "class Person {} class Animal {}";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 2);
  }
}
