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

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(std::move(program));
    return true;
  } catch (...) {
    return false;
  }
}

TEST_CASE("Exception: Try-catch statement", "[exception][try_catch]") {
  SECTION("Simple try-catch") {
    REQUIRE(analyzeSource("func test() { try { } catch (int e) { } }") == true);
  }
}

TEST_CASE("Exception: Catch all", "[exception][catch_all]") {
  SECTION("Catch all with ...") {
    REQUIRE(analyzeSource("func test() { try { } catch (...) { } }") == true);
  }
}

TEST_CASE("Exception: Try-catch with return", "[exception][return]") {
  SECTION("Return in try") {
    REQUIRE(analyzeSource("func test() -> int { try { return 1; } catch (...) "
                          "{ return 0; } }") == true);
  }
}
