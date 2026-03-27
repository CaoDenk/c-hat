#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    std::string sourceWithMain = source + "\nfunc main() { }\n";
    parser::Parser parser(sourceWithMain);
    auto program = parser.parseProgram();
    if (!program) {
      std::cerr << "Parse failed" << std::endl;
      return false;
    }

    semantic::SemanticAnalyzer analyzer;
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
