#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include "../src/semantic/TypeMetadata.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program)
      return false;

    semantic::SemanticAnalyzer analyzer("", false);
    analyzer.analyze(*program);
    return !analyzer.hasError();
  } catch (...) {
    return false;
  }
}

// ============================================
// Phase 1: is keyword tests
// ============================================
TEST_CASE("Reflection: is keyword", "[reflection][is]") {
  SECTION("struct is struct") {
    REQUIRE(analyzeSource("struct Point { int x; int y; } "
                          "func test() { if (Point is struct) { } }") == true);
  }

  SECTION("class is class") {
    REQUIRE(analyzeSource("class Animal { } "
                          "func test() { if (Animal is class) { } }") == true);
  }

  SECTION("enum is enum") {
    REQUIRE(analyzeSource("enum Color { Red, Green, Blue } "
                          "func test() { if (Color is enum) { } }") == true);
  }
}

// ============================================
// Phase 2: typeof tests
// ============================================
TEST_CASE("Reflection: typeof expression", "[reflection][typeof]") {
  SECTION("typeof with variable") {
    REQUIRE(analyzeSource("func test() { var x = 42; var t = typeof(x); }") == true);
  }

  SECTION("typeof with literal") {
    REQUIRE(analyzeSource("func test() { var t = typeof(42); }") == true);
  }
}

// ============================================
// Phase 3: metadata collection tests
// ============================================
TEST_CASE("Reflection: metadata collection", "[reflection][metadata]") {
  SECTION("struct metadata") {
    REQUIRE(analyzeSource("struct Point { public int x; public int y; } "
                          "func main() { }") == true);
  }

  SECTION("class metadata") {
    REQUIRE(analyzeSource("class Animal { public string name; public int age; } "
                          "func main() { }") == true);
  }

  SECTION("enum metadata") {
    REQUIRE(analyzeSource("enum Direction { North, South, East, West } "
                          "func main() { }") == true);
  }
}

// ============================================
// Phase 4: reflection expression tests
// ============================================
TEST_CASE("Reflection: reflection expression", "[reflection][expr]") {
  SECTION("@Type expression") {
    REQUIRE(analyzeSource("func test() { var t = @int; }") == true);
  }

  SECTION("@Typeof expression") {
    REQUIRE(analyzeSource("func test() { var x = 42; var t = @typeof(x); }") == true);
  }
}

// ============================================
// Phase 5: comptime for tests
// ============================================
TEST_CASE("Reflection: comptime for", "[reflection][comptime]") {
  SECTION("comptime for basic") {
    REQUIRE(analyzeSource("func test() { "
                          "comptime for (var i = 0; i < 3; i = i + 1) { "
                          "  var x = i; "
                          "} }") == true);
  }

  SECTION("comptime statement") {
    REQUIRE(analyzeSource("func test() { "
                          "comptime { var x = 0; } "
                          "}") == true);
  }
}

// ============================================
// Phase 6: comptime if tests
// ============================================
TEST_CASE("Reflection: comptime if", "[reflection][comptime]") {
  SECTION("comptime if basic") {
    REQUIRE(analyzeSource("func test() { "
                          "comptime if (true) { "
                          "  var x = 1; "
                          "} }") == true);
  }

  SECTION("comptime if with else") {
    REQUIRE(analyzeSource("func test() { "
                          "comptime if (false) { "
                          "  var x = 1; "
                          "} else { "
                          "  var x = 2; "
                          "} }") == true);
  }
}

// ============================================
// Phase 7: obj.[field] tests
// ============================================
TEST_CASE("Reflection: meta field access", "[reflection][meta]") {
  SECTION("basic meta field access") {
    REQUIRE(analyzeSource("struct Point { public int x; public int y; } "
                          "func test() { "
                          "  var p = Point(); "
                          "  var val = p.[0]; "
                          "}") == true);
  }
}
