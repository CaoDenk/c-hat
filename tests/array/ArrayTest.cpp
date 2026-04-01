#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    // 为测试代码添加 main 函数包装
    std::string wrappedSource = "func main() { " + source + " }";
    parser::Parser parser(wrappedSource);
    auto program = parser.parseProgram();
    if (!program) {
      std::cout << "Parser failed" << std::endl;
      return false;
    }

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
    if (analyzer.hasError()) {
      std::cout << "Semantic analysis failed for: " << source << std::endl;
    }
    return !analyzer.hasError();
  } catch (const std::exception &e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return false;
  } catch (...) {
    return false;
  }
}

bool analyzeSourceWithMain(const std::string &source) {
  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program)
      return false;

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
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
    REQUIRE(analyzeSource("var arr = [1, 2, 3]; for (var i = 0; "
                          "i < 3; i = i + 1) { var x = arr[i]; }") == true);
  }
}

TEST_CASE("Array: Array as function parameter", "[array][function]") {
  SECTION("Array parameter") {
    REQUIRE(analyzeSourceWithMain(
                "func sum(int[] arr) -> int { return 0; } func main() { }") ==
            true);
  }
}

TEST_CASE("Array: var vs let", "[array][variable]") {
  SECTION("var can reassign") {
    REQUIRE(analyzeSource("var arr = [1, 2, 3]; arr = [4, 5, 6];") == true);
  }

  SECTION("let cannot reassign") {
    REQUIRE(analyzeSource("let arr = [1, 2, 3]; arr = [4, 5, 6];") == false);
  }

  SECTION("let can modify elements") {
    REQUIRE(analyzeSource("let arr = [1, 2, 3]; arr[0] = 99;") == true);
  }
}

TEST_CASE("Array: array assignment type check", "[array][type]") {
  SECTION("same size array can assign") {
    REQUIRE(analyzeSource("var arr = [1, 2, 3]; arr = [4, 5, 6];") == true);
  }

  SECTION("different size array cannot assign") {
    REQUIRE(analyzeSource("var arr = [1, 2, 3]; arr = [4, 5, 6, 7];") == false);
  }
}

TEST_CASE("Array: const array", "[array][const]") {
  SECTION("const array declaration") {
    REQUIRE(analyzeSource("const int[3] arr = [1, 2, 3];") == true);
  }

  SECTION("const array cannot modify elements") {
    REQUIRE(analyzeSource("const int[3] arr = [1, 2, 3]; arr[0] = 99;") ==
            false);
  }
}

TEST_CASE("Array: Array properties", "[array][properties]") {
  SECTION("Array len property") {
    REQUIRE(analyzeSource("int[3] arr = [1, 2, 3]; int len = arr.len;") ==
            true);
  }

  SECTION("Array ptr property") {
    REQUIRE(analyzeSource("int[3] arr = [1, 2, 3]; int^ ptr = arr.ptr;") ==
            true);
  }

  SECTION("Array len property returns int") {
    REQUIRE(analyzeSource("int[3] arr = [1, 2, 3]; int x = arr.len + 1;") ==
            true);
  }

  SECTION("Array ptr property is pointer type") {
    REQUIRE(analyzeSource("int[3] arr = [1, 2, 3]; int^ ptr = "
                          "arr.ptr; int x = ptr[0];") == true);
  }

  SECTION("Invalid array property should fail") {
    REQUIRE(analyzeSource("int[3] arr = [1, 2, 3]; int x = arr.invalid;") ==
            false);
  }
}

TEST_CASE("Array: Slice properties", "[array][slice][properties]") {
  SECTION("Slice len property") {
    REQUIRE(analyzeSource("int[] slice = [1, 2, 3]; int len = slice.len;") ==
            true);
  }

  SECTION("Slice ptr property") {
    REQUIRE(analyzeSource("int[] slice = [1, 2, 3]; int^ ptr = slice.ptr;") ==
            true);
  }

  SECTION("Slice len property returns int") {
    REQUIRE(analyzeSource("int[] slice = [1, 2, 3]; int x = "
                          "slice.len + 1;") == true);
  }

  SECTION("Slice ptr property is pointer type") {
    REQUIRE(analyzeSource("int[] slice = [1, 2, 3]; int^ ptr = "
                          "slice.ptr; int x = ptr[0];") == true);
  }
}

TEST_CASE("Array: Array to slice conversion", "[array][slice][conversion]") {
  SECTION("Array can be assigned to slice") {
    REQUIRE(analyzeSource("int[3] arr = [1, 2, 3]; int[] slice = arr;") ==
            true);
  }

  SECTION("Slice can reference array") {
    REQUIRE(analyzeSource("int[5] arr = [1, 2, 3, 4, 5]; int[] "
                          "slice = arr; int len = slice.len;") == true);
  }
}

TEST_CASE("Array: Error path tests", "[array][errors]") {
  SECTION("Array reassign with wrong size") {
    REQUIRE(analyzeSource("var arr = [1, 2, 3]; arr = [1, 2];") == false);
  }

  SECTION("Let variable reassignment") {
    REQUIRE(analyzeSource("let arr = [1, 2, 3]; arr = [4, 5, 6];") == false);
  }
}
