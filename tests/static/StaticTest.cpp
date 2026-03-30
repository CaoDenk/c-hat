// StaticTest.cpp - static 设计 (docs/design/static设计.md)
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
// 1. static 成员变量
// ─────────────────────────────────────────────
TEST_CASE("Static: static member variable", "[static][parser]") {
    SECTION("Static int member") {
        CHECK(analyzeSource(
            "class Counter {\n"
            "  public static int count = 0;\n"
            "}\n"
            "func main() { }") == true);
    }
    SECTION("Static member access via class name") {
        CHECK(analyzeSource(
            "class Counter {\n"
            "  public static int count = 0;\n"
            "}\n"
            "func main() { int c = Counter.count; }") == true);
    }
    SECTION("Static member cannot be accessed via instance") {
        // 按 C^ 设计，静态成员只能通过类名访问
        CHECK(analyzeSource(
            "class Counter {\n"
            "  public static int count = 0;\n"
            "}\n"
            "func main() { Counter obj; int c = obj.count; }") == false);
    }
}

// ─────────────────────────────────────────────
// 2. static 成员函数
// ─────────────────────────────────────────────
TEST_CASE("Static: static member function", "[static][semantic]") {
    SECTION("Static function declaration") {
        CHECK(analyzeSource(
            "class Math {\n"
            "  public static func square(int x) -> int { return x * x; }\n"
            "}\n"
            "func main() { }") == true);
    }
    SECTION("Static function call via class name") {
        CHECK(analyzeSource(
            "class Math {\n"
            "  public static func square(int x) -> int { return x * x; }\n"
            "}\n"
            "func main() { int r = Math.square(4); }") == true);
    }
    SECTION("Static function cannot access non-static members") {
        CHECK(analyzeSource(
            "class Foo {\n"
            "  int val;\n"
            "  public static func bad() -> int { return val; }\n"  // 应报错
            "}\n"
            "func main() { }") == false);
    }
}

// ─────────────────────────────────────────────
// 3. 全局 static（文件作用域）
// ─────────────────────────────────────────────
TEST_CASE("Static: global static variable", "[static][semantic]") {
    SECTION("Top-level static variable") {
        CHECK(analyzeSource(
            "static int g_count = 0;\n"
            "func main() { g_count = 1; }") == true);
    }
    SECTION("Static function local variable persists") {
        CHECK(analyzeSource(
            "func counter() -> int {\n"
            "  static int n = 0;\n"
            "  n = n + 1;\n"
            "  return n;\n"
            "}\n"
            "func main() { int x = counter(); }") == true);
    }
}
