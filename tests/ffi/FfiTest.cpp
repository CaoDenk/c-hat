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

TEST_CASE("FFI: Parse extern block", "[ffi][parse]") {
  SECTION("Parse simple extern C block") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func printf(byte^ fmt, ...) -> int; "
        "}") == true);
  }

  SECTION("Parse multiple functions in extern block") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func malloc(long size) -> byte^; "
        "  func free(byte^ ptr); "
        "}") == true);
  }

  SECTION("Parse extern with void return") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func exit(int code); "
        "}") == true);
  }
}

TEST_CASE("FFI: Type mapping", "[ffi][types]") {
  SECTION("Basic C types") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func test(byte a, sbyte b, short c, int d, long e); "
        "}") == true);
  }

  SECTION("Float types") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func test(float a, double b); "
        "}") == true);
  }

  SECTION("Pointer types") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func test(byte^ a, int^ b); "
        "}") == true);
  }
}

TEST_CASE("FFI: Use extern functions", "[ffi][usage]") {
  SECTION("Call extern function") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func abs(int x) -> int; "
        "} "
        "func test() -> int { "
        "  return abs(-5); "
        "}") == true);
  }

  SECTION("Use extern function with pointer") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func malloc(long size) -> byte^; "
        "  func free(byte^ ptr); "
        "} "
        "func test() { "
        "  byte^ p = malloc(1024); "
        "  free(p); "
        "}") == true);
  }
}

TEST_CASE("FFI: Variadic functions", "[ffi][variadic]") {
  SECTION("Printf style variadic") {
    REQUIRE(analyzeSource(
        "extern \"C\" { "
        "  func printf(byte^ fmt, ...) -> int; "
        "}") == true);
  }
}
