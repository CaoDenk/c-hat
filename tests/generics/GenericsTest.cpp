// GenericsTest.cpp - 泛型设计 (docs/design/泛型设计.md)
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

static bool analyzeSource(const std::string& source) {
    try {
        parser::Parser p(source);
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
// 1. 泛型函数声明
// ─────────────────────────────────────────────
TEST_CASE("Generics: generic function", "[generics][parser]") {
    SECTION("Simple generic function") {
        CHECK(analyzeSource("func identity<T>(T x) -> T { return x; } func main() { }") == true);
    }
    SECTION("Two type parameters") {
        CHECK(analyzeSource("func pair<A, B>(A a, B b) -> A { return a; } func main() { }") == true);
    }
    SECTION("Generic function with where constraint") {
        CHECK(analyzeSource(
            "concept Printable<T> { }\n"
            "func print_val<T>(T x) where Printable<T> { }\n"
            "func main() { }") == true);
    }
}

// ─────────────────────────────────────────────
// 2. 泛型类
// ─────────────────────────────────────────────
TEST_CASE("Generics: generic class", "[generics][parser]") {
    SECTION("Generic class declaration") {
        CHECK(analyzeSource(
            "class Box<T> { public T value; }\n"
            "func main() { }") == true);
    }
    SECTION("Generic class instantiation") {
        CHECK(analyzeSource(
            "class Box<T> { public T value; }\n"
            "func main() { Box<int> b; }") == true);
    }
    SECTION("Nested generic") {
        CHECK(analyzeSource(
            "class Pair<A, B> { public A first; public B second; }\n"
            "func main() { Pair<int, float> p; }") == true);
    }
}

// ─────────────────────────────────────────────
// 3. Concept 定义
// ─────────────────────────────────────────────
TEST_CASE("Generics: concept definition", "[generics][parser]") {
    SECTION("Empty concept") {
        CHECK(analyzeSource("concept Any<T> { } func main() { }") == true);
    }
    SECTION("Concept with where clause") {
        CHECK(analyzeSource(
            "concept Integral<T> where typeof(T) == typeof(int) || typeof(T) == typeof(long);\n"
            "func main() { }") == true);
    }
}

// ─────────────────────────────────────────────
// 4. using 类型集合约束（| 语法糖）
// ─────────────────────────────────────────────
TEST_CASE("Generics: type set alias (| syntax)", "[generics][alias]") {
    SECTION("Basic type set alias") {
        CHECK(analyzeSource(
            "using Numeric = int | long | float | double;\n"
            "func main() { }") == true);
    }
    SECTION("Type set used in where clause") {
        CHECK(analyzeSource(
            "using Integral = int | long;\n"
            "func double_val<T>(T x) -> T where Integral<T> { return x; }\n"
            "func main() { }") == true);
    }
    SECTION("Type set cannot be used as variable type") {
        // Numeric x = 1; 应该是编译错误
        CHECK(analyzeSource(
            "using Numeric = int | long;\n"
            "func main() { Numeric x = 1; }") == false);
    }
}

// ─────────────────────────────────────────────
// 5. 泛型实例化
// ─────────────────────────────────────────────
TEST_CASE("Generics: instantiation", "[generics][semantic]") {
    SECTION("Call generic function with explicit type") {
        CHECK(analyzeSource(
            "func identity<T>(T x) -> T { return x; }\n"
            "func main() { int y = identity<int>(42); }") == true);
    }
    SECTION("Generic stack-like class usage") {
        CHECK(analyzeSource(
            "class Stack<T> { public func push(T item) { } }\n"
            "func main() { Stack<int> s; s.push(1); }") == true);
    }
}
