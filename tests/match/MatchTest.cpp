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

TEST_CASE("Match: Basic match with identifier", "[match][basic]") {
  SECTION("Match with single identifier pattern") {
    REQUIRE(analyzeSource(
        "func test() { var x = 1; match (x) { y => { } }; }") == true);
  }
}

TEST_CASE("Match: Basic match with integer", "[match][basic]") {
  SECTION("Match with single integer pattern") {
    REQUIRE(analyzeSource(
        "func test() { var x = 1; match (x) { 1 => { } }; }") == true);
  }
}

TEST_CASE("Match: guards and bindings", "[match][semantic]") {
  SECTION("Bool guard is accepted") {
    REQUIRE(analyzeSource(
                "func test() { var x = 1; match (x) { 1 if true => { } _ => { } }; }") == true);
  }

  SECTION("Non-bool guard is rejected") {
    REQUIRE(analyzeSource(
                "func test() { var x = 1; match (x) { 1 if 1 => { } _ => { } }; }") == false);
  }

  SECTION("Undefined guard identifier is rejected") {
    REQUIRE(analyzeSource(
                "func test() { var x = 1; match (x) { 1 if missing => { } _ => { } }; }") == false);
  }

  SECTION("Binding is visible in guard and body") {
    REQUIRE(analyzeSource(
                "func test() -> int { var x = 1; var r = 0; match (x) { y if y > 0 => { r = y; } _ => { } }; return r; }") == true);
  }

  SECTION("Literal pattern type mismatch is rejected") {
    REQUIRE(analyzeSource(
                "func test() { var x = 1; match (x) { \"bad\" => { } _ => { } }; }") == false);
  }
}
