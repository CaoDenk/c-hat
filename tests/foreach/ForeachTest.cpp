// ForeachTest.cpp - foreach 设计 (docs/design/foreach设计.md)
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

static bool analyzeSource(const std::string& src) {
    try {
        parser::Parser p(src);
        auto prog = p.parseProgram();
        if (!prog) return false;
        semantic::SemanticAnalyzer analyzer("", false);
        analyzer.analyze(*prog);
        return !analyzer.hasError();
    } catch (...) {
        return false;
    }
}

static bool analyzeInMain(const std::string& body) {
    return analyzeSource("func main() { " + body + " }");
}

// ─────────────────────────────────────────────
// 1. 基本 foreach 语句
// ─────────────────────────────────────────────
TEST_CASE("Foreach: basic iteration", "[foreach][parser]") {
    SECTION("Foreach over array literal") {
        // foreach (var x : arr) { }
        CHECK(analyzeSource(
            "func main() { "
            "  int[3] arr = [1, 2, 3]; "
            "  foreach (var x : arr) { } "
            "}") == true);
    }
    SECTION("Foreach over slice") {
        CHECK(analyzeSource(
            "func test(int[] s) { "
            "  foreach (var item : s) { } "
            "} "
            "func main() { }") == true);
    }
    SECTION("Foreach with let") {
        CHECK(analyzeSource(
            "func main() { "
            "  int[3] arr = [1, 2, 3]; "
            "  foreach (let x : arr) { } "
            "}") == true);
    }
}

// ─────────────────────────────────────────────
// 2. foreach 变量作用域
// ─────────────────────────────────────────────
TEST_CASE("Foreach: variable scope", "[foreach][semantic]") {
    SECTION("Loop variable scoped to block") {
        // x 只在 foreach 块内可见
        CHECK(analyzeSource(
            "func main() { "
            "  int[3] arr = [1, 2, 3]; "
            "  foreach (var x : arr) { int y = x; } "
            "}") == true);
    }
    SECTION("Loop variable not visible after block") {
        CHECK(analyzeSource(
            "func main() { "
            "  int[3] arr = [1, 2, 3]; "
            "  foreach (var x : arr) { } "
            "  int z = x; "   // x 超出作用域，应报错
            "}") == false);
    }
}

// ─────────────────────────────────────────────
// 3. 带索引的 foreach
// ─────────────────────────────────────────────
TEST_CASE("Foreach: with index", "[foreach][parser]") {
    SECTION("Foreach with index variable") {
        CHECK(analyzeSource(
            "func main() { "
            "  int[3] arr = [1, 2, 3]; "
            "  foreach (var i, var x : arr) { } "
            "}") == true);
    }
}

// ─────────────────────────────────────────────
// 4. break / continue 在 foreach 中
// ─────────────────────────────────────────────
TEST_CASE("Foreach: break and continue", "[foreach][semantic]") {
    SECTION("break in foreach") {
        CHECK(analyzeSource(
            "func main() { "
            "  int[3] arr = [1, 2, 3]; "
            "  foreach (var x : arr) { break; } "
            "}") == true);
    }
    SECTION("continue in foreach") {
        CHECK(analyzeSource(
            "func main() { "
            "  int[3] arr = [1, 2, 3]; "
            "  foreach (var x : arr) { continue; } "
            "}") == true);
    }
    SECTION("break outside loop should error") {
        CHECK(analyzeInMain("break;") == false);
    }
}
