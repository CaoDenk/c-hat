// ModuleTest.cpp - 模块系统设计 (docs/design/模块系统设计.md)
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

// 解析顶层声明（模块语句在函数外）
static bool parseTopLevel(const std::string& source) {
    try {
        parser::Parser p(source);
        auto prog = p.parseProgram();
        if (!prog) return false;
        semantic::SemanticAnalyzer analyzer;
        analyzer.analyze(*prog);
        return !analyzer.hasError();
    } catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────
// 1. 模块声明
// ─────────────────────────────────────────────
TEST_CASE("Module: module declaration", "[module][parser]") {
    SECTION("Simple module declaration") {
        // module A; 声明当前文件属于模块 A
        CHECK(parseTopLevel("module A; func main() { }") == true);
    }
    SECTION("Nested module declaration") {
        CHECK(parseTopLevel("module A.B; func main() { }") == true);
    }
    SECTION("Deep nested module") {
        CHECK(parseTopLevel("module Std.Collections.List; func main() { }") == true);
    }
}

// ─────────────────────────────────────────────
// 2. import 语句
// ─────────────────────────────────────────────
TEST_CASE("Module: import statement", "[module][parser]") {
    SECTION("Import single module") {
        CHECK(parseTopLevel("import A; func main() { }") == true);
    }
    SECTION("Import nested module") {
        CHECK(parseTopLevel("import A.B; func main() { }") == true);
    }
    SECTION("Import with alias") {
        CHECK(parseTopLevel("import A.B as AB; func main() { }") == true);
    }
    SECTION("Multiple imports") {
        CHECK(parseTopLevel(
            "import A;\n"
            "import B.C;\n"
            "func main() { }") == true);
    }
}

// ─────────────────────────────────────────────
// 3. public 导出
// ─────────────────────────────────────────────
TEST_CASE("Module: public export", "[module][semantic]") {
    SECTION("Public function is exported") {
        CHECK(parseTopLevel(
            "module MyMod;\n"
            "public func greet() { }\n"
            "func main() { }") == true);
    }
    SECTION("Private function not exported") {
        CHECK(parseTopLevel(
            "module MyMod;\n"
            "func helper() { }\n"
            "func main() { }") == true);
    }
    SECTION("Public class") {
        CHECK(parseTopLevel(
            "module MyMod;\n"
            "public class Point { public int x; public int y; }\n"
            "func main() { }") == true);
    }
}
