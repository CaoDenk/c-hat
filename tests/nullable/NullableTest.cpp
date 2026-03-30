// NullableTest.cpp - 可空类型设计 (docs/design/可空类型设计.md)
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

// ─────────────────────────────────────────────
// 1. 可空类型声明
// ─────────────────────────────────────────────
TEST_CASE("Nullable: type declaration", "[nullable][parser]") {
    SECTION("Nullable int") {
        CHECK(analyzeSource("func main() { int? x; }") == true);
    }
    SECTION("Nullable class") {
        CHECK(analyzeSource(
            "class Foo { }\n"
            "func main() { Foo? f; }") == true);
    }
    SECTION("Nullable pointer") {
        CHECK(analyzeSource("func main() { int?^ p; }") == true);
    }
}

// ─────────────────────────────────────────────
// 2. null 赋值
// ─────────────────────────────────────────────
TEST_CASE("Nullable: null assignment", "[nullable][semantic]") {
    SECTION("Assign null to nullable") {
        CHECK(analyzeSource("func main() { int? x = null; }") == true);
    }
    SECTION("Assign null to non-nullable should error") {
        CHECK(analyzeSource("func main() { int x = null; }") == false);
    }
    SECTION("Nullable can hold value") {
        CHECK(analyzeSource("func main() { int? x = 42; }") == true);
    }
}

// ─────────────────────────────────────────────
// 3. ? 传播操作符（用于可空类型）
// ─────────────────────────────────────────────
TEST_CASE("Nullable: ? propagation", "[nullable][semantic]") {
    SECTION("? on nullable type propagates") {
        CHECK(analyzeSource(
            "func maybe_int() -> int? { return null; }\n"
            "func caller() -> int? { "
            "  int x = maybe_int()?; "
            "  return x; "
            "}\n"
            "func main() { }") == true);
    }
    SECTION("? on non-nullable should error") {
        CHECK(analyzeSource(
            "func get_int() -> int { return 1; }\n"
            "func caller() -> int? { "
            "  int x = get_int()?; "   // 非可空类型用 ?，应报错
            "  return x; "
            "}\n"
            "func main() { }") == false);
    }
}

// ─────────────────────────────────────────────
// 4. ! 强制解包
// ─────────────────────────────────────────────
TEST_CASE("Nullable: ! force unwrap", "[nullable][semantic]") {
    SECTION("Force unwrap nullable") {
        CHECK(analyzeSource(
            "func maybe() -> int? { return 42; }\n"
            "func main() { int x = maybe()!; }") == true);
    }
    SECTION("Force unwrap non-nullable should error") {
        CHECK(analyzeSource(
            "func get() -> int { return 1; }\n"
            "func main() { int x = get()!; }") == false);
    }
}

// ─────────────────────────────────────────────
// 5. 可空类型函数返回
// ─────────────────────────────────────────────
TEST_CASE("Nullable: function return", "[nullable][semantic]") {
    SECTION("Function returning nullable") {
        CHECK(analyzeSource(
            "func find(int[] arr, int val) -> int? { return null; }\n"
            "func main() { }") == true);
    }
    SECTION("Return non-null from nullable return type") {
        CHECK(analyzeSource(
            "func maybe_value() -> int? { return 1; }\n"
            "func main() { }") == true);
    }
}
