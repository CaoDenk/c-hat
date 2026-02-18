#include "../src/parser/Parser.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

TEST_CASE("Tuple: Basic tuple literal", "[tuple][literal]") {
  SECTION("Two-element tuple") {
    std::string source = "func test() { var t = (1, \"hello\"); }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Three-element tuple") {
    std::string source = "func test() { var t = (3.14, true, 42); }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}
