#include "../src/parser/Parser.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

TEST_CASE("Lambda: Basic lambda", "[lambda][basic]") {
  SECTION("Lambda with fat arrow body and no captures/params") {
    std::string source = "func test() { var f = [] => 5; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Lambda with fat arrow, no captures and params") {
    std::string source = "func test() { var add = [](int a, int b) => a + b; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}
