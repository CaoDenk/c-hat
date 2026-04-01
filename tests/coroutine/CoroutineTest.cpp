#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <sstream>

static bool parseSource(const std::string &source) {
  try {
    c_hat::parser::Parser parser(source);
    auto program = parser.parseProgram();
    return program != nullptr;
  } catch (...) {
    return false;
  }
}

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

TEST_CASE("Coroutine: Parse yield statement", "[coroutine][parse]") {
  SECTION("Parse yield with value") {
    REQUIRE(parseSource("func gen() { "
                        "  yield 1; "
                        "  yield 2; "
                        "}") == true);
  }

  SECTION("Parse yield without value") {
    REQUIRE(parseSource("func gen() { "
                        "  yield; "
                        "}") == true);
  }

  SECTION("Parse yield in loop") {
    REQUIRE(parseSource("func range(int start, int end) { "
                        "  for (int i = start; i < end; i++) { "
                        "    yield i; "
                        "  } "
                        "}") == true);
  }
}

TEST_CASE("Coroutine: Parse await expression", "[coroutine][parse]") {
  SECTION("Parse simple await") {
    REQUIRE(parseSource("func fetch() { "
                        "  var data = await read_async(); "
                        "}") == true);
  }

  SECTION("Parse await in expression") {
    REQUIRE(parseSource("func compute() { "
                        "  int x = await get_x() + await get_y(); "
                        "}") == true);
  }

  SECTION("Parse chained await") {
    REQUIRE(parseSource("func chain() { "
                        "  var a = await first(); "
                        "  var b = await second(a); "
                        "  var c = await third(b); "
                        "}") == true);
  }
}

TEST_CASE("Coroutine: Combined yield and await", "[coroutine][parse]") {
  SECTION("Generator with async source") {
    REQUIRE(parseSource("func async_gen() { "
                        "  var data = await fetch_data(); "
                        "  for (var item : data) { "
                        "    yield item; "
                        "  } "
                        "}") == true);
  }
}

TEST_CASE("Coroutine: Semantic analysis", "[coroutine][semantic]") {
  SECTION("Yield statement semantic check") {
    REQUIRE(analyzeSource("func gen() { "
                          "  yield 1; "
                          "}") == true);
  }

  SECTION("Await expression semantic check") {
    REQUIRE(analyzeSource("func read() -> int { return 0; } "
                          "func fetch() { "
                          "  var x = await read(); "
                          "}") == true);
  }

  SECTION("Yield in nested control flow") {
    REQUIRE(analyzeSource("func gen_with_condition() { "
                          "  if (true) { "
                          "    yield 1; "
                          "  } else { "
                          "    yield 2; "
                          "  } "
                          "}") == true);
  }

  SECTION("Await in nested expressions") {
    REQUIRE(analyzeSource("func get_a() -> int { return 1; } "
                          "func get_b() -> int { return 2; } "
                          "func compute() { "
                          "  var result = await get_a() + await get_b(); "
                          "}") == true);
  }

  SECTION("Generator with loop") {
    REQUIRE(analyzeSource("func counter(int n) { "
                          "  for (int i = 0; i < n; i++) { "
                          "    yield i; "
                          "  } "
                          "}") == true);
  }

  SECTION("Async generator pattern") {
    REQUIRE(analyzeSource("func fetch_item(int id) -> int { return id; } "
                          "func async_gen(int count) { "
                          "  for (int i = 0; i < count; i++) { "
                          "    var item = await fetch_item(i); "
                          "    yield item; "
                          "  } "
                          "}") == true);
  }
}
