// ReferenceTest.cpp - 引用设计 (docs/design/引用设计.md)
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
// 1. 不可变引用参数（隐式传递）
// ─────────────────────────────────────────────
TEST_CASE("Reference: immutable reference param", "[reference][parser]") {
    SECTION("Function with immutable ref param") {
        // func foo(int!& x) — 不可变引用，调用时不需要 &x
        CHECK(analyzeSource(
            "func foo(int!& x) -> int { return x; }\n"
            "func main() { int a = 10; int b = foo(a); }") == true);
    }
    SECTION("Immutable ref cannot be assigned") {
        CHECK(analyzeSource(
            "func foo(int!& x) { x = 5; }\n"    // 修改不可变引用，应报错
            "func main() { int a = 1; foo(a); }") == false);
    }
}

// ─────────────────────────────────────────────
// 2. 可变引用参数（调用处必须显式 &x）
// ─────────────────────────────────────────────
TEST_CASE("Reference: mutable reference param", "[reference][parser]") {
    SECTION("Mutable ref param declaration") {
        CHECK(analyzeSource(
            "func increment(int& x) { x = x + 1; }\n"
            "func main() { int a = 0; increment(&a); }") == true);
    }
    SECTION("Mutable ref without & at callsite should error") {
        CHECK(analyzeSource(
            "func increment(int& x) { x = x + 1; }\n"
            "func main() { int a = 0; increment(a); }") == false);
    }
}

// ─────────────────────────────────────────────
// 3. 引用绑定
// ─────────────────────────────────────────────
TEST_CASE("Reference: reference binding", "[reference][semantic]") {
    SECTION("Local ref variable") {
        CHECK(analyzeSource(
            "func main() { int a = 1; int& r = &a; }") == true);
    }
    SECTION("Immutable local ref") {
        CHECK(analyzeSource(
            "func main() { int a = 1; int!& r = a; }") == true);
    }
}

// ─────────────────────────────────────────────
// 4. 引用作为返回类型
// ─────────────────────────────────────────────
TEST_CASE("Reference: return by reference", "[reference][semantic]") {
    SECTION("Function returning ref") {
        CHECK(analyzeSource(
            "func first(int[]& arr) -> int& { return arr[0]; }\n"
            "func main() { }") == true);
    }
}
