#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <sstream>

static bool analyzeSource(const std::string &source) {
  try {
    c_hat::parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      return false;
    }

    c_hat::semantic::SemanticAnalyzer analyzer("", false);
    analyzer.analyze(*program);
    return !analyzer.hasError();
  } catch (...) {
    return false;
  }
}

TEST_CASE("Goto: Parse simple goto and label", "[goto][parse]") {
  SECTION("Parse goto statement") {
    REQUIRE(analyzeSource("func test() { "
                          "  goto end; "
                          "  end: ; "
                          "}") == true);
  }

  SECTION("Parse forward jump") {
    REQUIRE(analyzeSource("func forward() -> int { "
                          "  int x = 1; "
                          "  goto skip; "
                          "  x = 2; "
                          "  skip: ; "
                          "  return x; "
                          "}") == true);
  }

  SECTION("Parse backward jump") {
    REQUIRE(analyzeSource("func backward() { "
                          "  int i = 0; "
                          "  loop: ; "
                          "  i = i + 1; "
                          "  if (i < 10) goto loop; "
                          "}") == true);
  }
}

TEST_CASE("Goto: Error handling", "[goto][error]") {
  SECTION("Undefined label") {
    REQUIRE(analyzeSource("func test() { "
                          "  goto undefined_label; "
                          "}") == false);
  }

  SECTION("Multiple labels with same name") {
    REQUIRE(analyzeSource("func test() { "
                          "  label: ; "
                          "  label: ; "
                          "}") == false);
  }
}

TEST_CASE("Goto: Use cases", "[goto][usage]") {
  SECTION("Error handling pattern") {
    REQUIRE(analyzeSource("func doWork() -> int { return 0; } "
                          "func doMore() -> int { return 0; } "
                          "func process() -> int { "
                          "  int result = 0; "
                          "  result = doWork(); "
                          "  if (result < 0) goto error; "
                          "  result = doMore(); "
                          "  if (result < 0) goto error; "
                          "  return 0; "
                          "  error: ; "
                          "  return -1; "
                          "}") == true);
  }

  SECTION("State machine pattern") {
    REQUIRE(analyzeSource("func stateMachine() { "
                          "  int state = 0; "
                          "  state_0: ; "
                          "  state = 1; "
                          "  if (state == 1) goto state_1; "
                          "  goto end; "
                          "  state_1: ; "
                          "  state = 2; "
                          "  if (state == 2) goto end; "
                          "  goto state_0; "
                          "  end: ; "
                          "}") == true);
  }
}

TEST_CASE("Goto: Multiple labels", "[goto][multiple]") {
  SECTION("Multiple different labels") {
    REQUIRE(analyzeSource("func test() { "
                          "  start: ; "
                          "  middle: ; "
                          "  end: ; "
                          "  goto start; "
                          "}") == true);
  }
}
