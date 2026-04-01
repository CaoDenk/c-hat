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

    semantic::SemanticAnalyzer analyzer("", false);
    analyzer.analyze(*program);
    return !analyzer.hasError();
  } catch (...) {
    return false;
  }
}

TEST_CASE("Pointer: Basic pointer declaration", "[pointer][basic]") {
  SECTION("Int pointer") {
    REQUIRE(analyzeSource("func test() { int^ ptr; }") == true);
  }

  SECTION("Float pointer") {
    REQUIRE(analyzeSource("func test() { float^ ptr; }") == true);
  }

  SECTION("Double pointer") {
    REQUIRE(analyzeSource("func test() { double^ ptr; }") == true);
  }

  SECTION("Char pointer") {
    REQUIRE(analyzeSource("func test() { char^ ptr; }") == true);
  }

  SECTION("Bool pointer") {
    REQUIRE(analyzeSource("func test() { bool^ ptr; }") == true);
  }
}

TEST_CASE("Pointer: Pointer to pointer", "[pointer][double]") {
  SECTION("Double pointer") {
    REQUIRE(analyzeSource("func test() { int^^ ptr; }") == true);
  }

  SECTION("Triple pointer") {
    REQUIRE(analyzeSource("func test() { int^^^ ptr; }") == true);
  }
}

TEST_CASE("Pointer: Pointer initialization", "[pointer][init]") {
  SECTION("Pointer from address-of") {
    REQUIRE(analyzeSource("func test() { int x = 10; int^ ptr = ^x; }") == true);
  }

  SECTION("Pointer from array") {
    REQUIRE(analyzeSource("func test() { int[3] arr = [1, 2, 3]; int^ ptr = arr.ptr; }") == true);
  }

  SECTION("Pointer from slice") {
    REQUIRE(analyzeSource("func test() { int[] slice = [1, 2, 3]; int^ ptr = slice.ptr; }") == true);
  }
}

TEST_CASE("Pointer: Pointer dereferencing", "[pointer][deref]") {
  SECTION("Dereference pointer") {
    REQUIRE(analyzeSource("func test() { int x = 10; int^ ptr = ^x; int y = ptr^; }") == true);
  }

  SECTION("Dereference and modify") {
    REQUIRE(analyzeSource("func test() { int x = 10; int^ ptr = ^x; ptr^ = 20; }") == true);
  }

  SECTION("Double dereference") {
    REQUIRE(analyzeSource("func test() { int x = 10; int^ ptr1 = ^x; int^^ ptr2 = ^ptr1; int y = ptr2^^; }") == true);
  }
}

TEST_CASE("Pointer: Pointer arithmetic", "[pointer][arithmetic]") {
  SECTION("Pointer increment") {
    REQUIRE(analyzeSource("func test() { int[3] arr = [1, 2, 3]; int^ ptr = arr.ptr; ptr = ptr + 1; }") == true);
  }

  SECTION("Pointer decrement") {
    REQUIRE(analyzeSource("func test() { int[3] arr = [1, 2, 3]; int^ ptr = arr.ptr + 2; ptr = ptr - 1; }") == true);
  }

  SECTION("Pointer difference") {
    REQUIRE(analyzeSource("func test() { int[5] arr = [1, 2, 3, 4, 5]; int^ ptr1 = arr.ptr; int^ ptr2 = arr.ptr + 3; int diff = ptr2 - ptr1; }") == true);
  }

  SECTION("Pointer indexing") {
    REQUIRE(analyzeSource("func test() { int[3] arr = [1, 2, 3]; int^ ptr = arr.ptr; int x = ptr[1]; }") == true);
  }
}

TEST_CASE("Pointer: Pointer comparison", "[pointer][comparison]") {
  SECTION("Pointer equality") {
    REQUIRE(analyzeSource("func test() { int x = 10; int^ ptr1 = ^x; int^ ptr2 = ^x; bool eq = ptr1 == ptr2; }") == true);
  }

  SECTION("Pointer inequality") {
    REQUIRE(analyzeSource("func test() { int x = 10; int y = 20; int^ ptr1 = ^x; int^ ptr2 = ^y; bool neq = ptr1 != ptr2; }") == true);
  }

  SECTION("Pointer less than") {
    REQUIRE(analyzeSource("func test() { int[2] arr = [1, 2]; int^ ptr1 = arr.ptr; int^ ptr2 = arr.ptr + 1; bool lt = ptr1 < ptr2; }") == true);
  }
}

TEST_CASE("Pointer: Null pointer", "[pointer][null]") {
  SECTION("Null pointer initialization") {
    REQUIRE(analyzeSource("func test() { int^ ptr = null; }") == true);
  }

  SECTION("Null pointer check") {
    REQUIRE(analyzeSource("func test() { int^ ptr = null; bool isNull = ptr == null; }") == true);
  }

  SECTION("Null pointer assignment") {
    REQUIRE(analyzeSource("func test() { int x = 10; int^ ptr = ^x; ptr = null; }") == true);
  }
}

TEST_CASE("Pointer: Pointer to class", "[pointer][class]") {
  SECTION("Class pointer declaration") {
    REQUIRE(analyzeSource("class Person { } func test() { Person^ ptr; }") == true);
  }

  SECTION("Class pointer from new") {
    REQUIRE(analyzeSource("class Person { } func test() { Person^ ptr = new Person(); }") == true);
  }

  SECTION("Class pointer member access") {
    REQUIRE(analyzeSource("class Person { public int age; } func test() { Person^ ptr = new Person(); ptr.age = 25; }") == true);
  }

  SECTION("Class pointer method call") {
    REQUIRE(analyzeSource("class Person { public void greet() { } } func test() { Person^ ptr = new Person(); ptr.greet(); }") == true);
  }
}

TEST_CASE("Pointer: Pointer as function parameter", "[pointer][function]") {
  SECTION("Pointer parameter") {
    REQUIRE(analyzeSource("func modify(int^ ptr) { ptr^ = 10; }") == true);
  }

  SECTION("Pointer return type") {
    REQUIRE(analyzeSource("func getPtr(int^ ptr) -> int^ { return ptr; }") == true);
  }

  SECTION("Pass pointer to function") {
    REQUIRE(analyzeSource("func modify(int^ ptr) { ptr^ = 10; } func test() { int x = 5; modify(^x); }") == true);
  }
}

TEST_CASE("Pointer: Readonly pointer", "[pointer][readonly]") {
  SECTION("Pointer to readonly int") {
    // int!^ = 指向不可变 int 的指针 (类似 const int*)
    REQUIRE(analyzeSource("func test() { int x = 10; int!^ ptr = ^x; }") == true);
  }

  SECTION("Readonly pointer cannot modify") {
    // 通过 int!^ 修改值应该报错
    REQUIRE(analyzeSource("func test() { int x = 10; int!^ ptr = ^x; ptr^ = 20; }") == false);
  }

  SECTION("Readonly variable address") {
    // int! 变量的地址应该赋给 int!^
    REQUIRE(analyzeSource("func test() { int! x = 10; int!^ ptr = ^x; }") == true);
  }

  SECTION("Readonly var address to mutable pointer") {
    // int! 变量的地址不能赋给 int^（会绕过只读限制）
    REQUIRE(analyzeSource("func test() { int! x = 10; int^ ptr = ^x; }") == false);
  }
}

TEST_CASE("Pointer: Error cases", "[pointer][errors]") {
  SECTION("Dereference non-pointer") {
    REQUIRE(analyzeSource("func test() { int x = 10; int y = x^; }") == false);
  }

  SECTION("Assign wrong type to pointer") {
    REQUIRE(analyzeSource("func test() { int^ ptr = 10; }") == false);
  }

  SECTION("Pointer type mismatch") {
    REQUIRE(analyzeSource("func test() { int x = 10; float^ ptr = ^x; }") == false);
  }
}
