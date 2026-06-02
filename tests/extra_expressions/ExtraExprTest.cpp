// ExtraExprTest.cpp - 测试 FoldExpr / TypeIsExpr / ExpansionExpr / MetaFieldAccessExpr
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

static bool analyzeSource(const std::string &source) {
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

// ─────────────────────────────────────────────
// 1. TypeIsExpr — 类型判断 (x is kind)
// ─────────────────────────────────────────────
TEST_CASE("TypeIsExpr: type checking", "[extra][type_is]") {
  SECTION("Basic type kind check") {
    CHECK(analyzeSource(
        "func test() { var x = 42; var b = x is integer; }\n"
        "func main() { }") == true);
  }
  SECTION("String kind check") {
    CHECK(analyzeSource(
        "func test() { var s = \"hello\"; var b = s is string; }\n"
        "func main() { }") == true);
  }
  SECTION("Pointer kind check") {
    CHECK(analyzeSource(
        "func test() { var p = ^42; var b = p is pointer; }\n"
        "func main() { }") == true);
  }
  SECTION("Float kind check") {
    CHECK(analyzeSource(
        "func test() { var f = 3.14; var b = f is float; }\n"
        "func main() { }") == true);
  }
}

// ─────────────────────────────────────────────
// 2. ExpansionExpr — 展开表达式 (expr...)
// ─────────────────────────────────────────────
TEST_CASE("ExpansionExpr: parameter expansion", "[extra][expansion]") {
  SECTION("Variable expansion") {
    CHECK(analyzeSource(
        "func f<T...>(T... args) { }\n"
        "func main() { }") == true);
  }
}

// ─────────────────────────────────────────────
// 3. FoldExpr — 折叠表达式
// ─────────────────────────────────────────────
TEST_CASE("FoldExpr: fold expression", "[extra][fold]") {
  SECTION("Simple fold expression") {
    CHECK(analyzeSource(
        "func sum<T...>(T... args) { var r = (... + args); }\n"
        "func main() { }") == true);
  }
}

// ─────────────────────────────────────────────
// 4. MetaFieldAccessExpr — 元字段访问 (obj.[field])
// ─────────────────────────────────────────────
TEST_CASE("MetaFieldAccessExpr: meta field access", "[extra][meta_field]") {
  SECTION("Basic meta field access") {
    CHECK(analyzeSource(
        "func test() { var x = 42; var f = x.[field]; }\n"
        "func main() { }") == true);
  }
}
