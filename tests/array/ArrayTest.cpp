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
    return !analyzer.hasError();
  } catch (...) {
    return false;
  }
}

TEST_CASE("Array: Array type declaration", "[array][type]") {
  SECTION("Fixed size array") {
    REQUIRE(analyzeSource("int[10] arr;") == true);
  }

  SECTION("Slice type") { REQUIRE(analyzeSource("int[] slice;") == true); }
}

TEST_CASE("Array: Array literal", "[array][literal]") {
  SECTION("Integer array literal") {
    REQUIRE(analyzeSource("var arr = [1, 2, 3, 4, 5];") == true);
  }

  SECTION("String array literal") {
    REQUIRE(analyzeSource("var arr = [\"a\", \"b\", \"c\"];") == true);
  }

  SECTION("Empty array literal") {
    REQUIRE(analyzeSource("var arr = [];") == true);
  }
}

TEST_CASE("Array: Array with loop", "[array][loop]") {
  SECTION("Iterate array") {
    REQUIRE(analyzeSource("func test() { var arr = [1, 2, 3]; for (var i = 0; "
                          "i < 3; i = i + 1) { var x = arr[i]; } }") == true);
  }
}

TEST_CASE("Array: Array as function parameter", "[array][function]") {
  SECTION("Array parameter") {
    REQUIRE(analyzeSource("func sum(int[] arr) -> int { return 0; }") == true);
  }
}

TEST_CASE("Array: var vs let", "[array][variable]") {
  SECTION("var can reassign") {
    REQUIRE(analyzeSource(
                "func test() { var arr = [1, 2, 3]; arr = [4, 5, 6]; }") ==
            true);
  }

  SECTION("let cannot reassign") {
    REQUIRE(analyzeSource(
                "func test() { let arr = [1, 2, 3]; arr = [4, 5, 6]; }") ==
            false);
  }

  SECTION("let can modify elements") {
    REQUIRE(analyzeSource(
                "func test() { let arr = [1, 2, 3]; arr[0] = 99; }") == true);
  }
}

TEST_CASE("Array: array assignment type check", "[array][type]") {
  SECTION("same size array can assign") {
    REQUIRE(analyzeSource(
                "func test() { var arr = [1, 2, 3]; arr = [4, 5, 6]; }") ==
            true);
  }

  SECTION("different size array cannot assign") {
    REQUIRE(analyzeSource(
                "func test() { var arr = [1, 2, 3]; arr = [4, 5, 6, 7]; }") ==
            false);
  }
}

TEST_CASE("Array: const array", "[array][const]") {
  SECTION("const array declaration") {
    REQUIRE(analyzeSource("func test() { const int[3] arr = [1, 2, 3]; }") ==
            true);
  }
}
