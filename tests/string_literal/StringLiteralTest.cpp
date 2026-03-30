// StringLiteralTest.cpp - 字符串与 literalview 设计
// (docs/design/字符串设计.md, literalview设计.md, 字符与字符串设计.md)
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
// 1. literalview — 字符串字面量类型
// ─────────────────────────────────────────────
TEST_CASE("String: literalview type", "[string][parser]") {
    SECTION("String literal infers to literalview") {
        // var s = "hello" 推导为 literalview
        CHECK(analyzeSource("func main() { var s = \"hello\"; }") == true);
    }
    SECTION("Explicit literalview declaration") {
        CHECK(analyzeSource("func main() { literalview s = \"world\"; }") == true);
    }
    SECTION("literalview is immutable — cannot assign") {
        // literalview 是只读的，不能被修改
        CHECK(analyzeSource(
            "func main() { literalview s = \"a\"; s = \"b\"; }") == false);
    }
}

// ─────────────────────────────────────────────
// 2. char 类型
// ─────────────────────────────────────────────
TEST_CASE("String: char type", "[string][parser]") {
    SECTION("Char literal") {
        CHECK(analyzeSource("func main() { char c = 'A'; }") == true);
    }
    SECTION("Char in expression") {
        CHECK(analyzeSource("func main() { char c = 'a'; int n = c; }") == true);
    }
}

// ─────────────────────────────────────────────
// 3. literalview 作为函数参数
// ─────────────────────────────────────────────
TEST_CASE("String: literalview as parameter", "[string][semantic]") {
    SECTION("Function accepting literalview") {
        CHECK(analyzeSource(
            "func print(literalview s) { }\n"
            "func main() { print(\"hello\"); }") == true);
    }
    SECTION("Pass variable literalview") {
        CHECK(analyzeSource(
            "func print(literalview s) { }\n"
            "func main() { var s = \"hi\"; print(s); }") == true);
    }
}

// ─────────────────────────────────────────────
// 4. 转义字符
// ─────────────────────────────────────────────
TEST_CASE("String: escape sequences", "[string][lexer]") {
    SECTION("Newline escape") {
        CHECK(analyzeSource("func main() { var s = \"line1\\nline2\"; }") == true);
    }
    SECTION("Tab escape") {
        CHECK(analyzeSource("func main() { var s = \"col1\\tcol2\"; }") == true);
    }
    SECTION("Quote escape") {
        CHECK(analyzeSource("func main() { var s = \"say \\\"hi\\\"\"; }") == true);
    }
}

// ─────────────────────────────────────────────
// 5. byte 类型（字节）
// ─────────────────────────────────────────────
TEST_CASE("String: byte type", "[string][types]") {
    SECTION("byte variable") {
        CHECK(analyzeSource("func main() { byte b = 65; }") == true);
    }
    SECTION("byte pointer (C-style string)") {
        CHECK(analyzeSource("func main() { byte^ p; }") == true);
    }
}
