#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace c_hat;

bool analyzeSource(const std::string &source) {
  try {
    // 添加一个简单的 main 函数
    std::string sourceWithMain = source + "\nfunc main() { }\n";
    parser::Parser parser(sourceWithMain);
    auto program = parser.parseProgram();
    if (!program)
      return false;

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(*program);
    return !analyzer.hasError();
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return false;
  }
}

TEST_CASE("Class: Basic class declaration", "[class][basic]") {
  SECTION("Empty class") {
    std::string source = "class Person {}";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
  }
}

TEST_CASE("Class: Access modifiers", "[class][access]") {
  SECTION("Public class") {
    std::string source = "public class Person {}";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Private class") {
    std::string source = "private class Person {}";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Class: Class with members", "[class][members]") {
  SECTION("Class with member variables") {
    std::string source = "class Person { var name; var age; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Class with member functions") {
    std::string source = "class Person { func greet() { } }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Class: Member access modifiers", "[class][members][access]") {
  SECTION("Public member") {
    std::string source = "class Person { public var name; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Private member") {
    std::string source = "class Person { private var name; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }

  SECTION("Protected member") {
    std::string source = "class Person { protected var name; }";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
  }
}

TEST_CASE("Class: Multiple classes", "[class][multiple]") {
  SECTION("Two classes") {
    std::string source = "class Person {} class Animal {}";
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 2);
  }
}

TEST_CASE("Class: Constructor", "[class][constructor]") {
  SECTION("Class with constructor") {
    REQUIRE(analyzeSource("class Person { Person() { } }") == true);
  }

  SECTION("Class with parameterized constructor") {
    REQUIRE(analyzeSource("class Person { Person(int id, int age) { } }") ==
            true);
  }
}

TEST_CASE("Class: Destructor", "[class][destructor]") {
  SECTION("Class with destructor") {
    REQUIRE(analyzeSource("class Person { ~Person() { } }") == true);
  }
}

TEST_CASE("Class: Constructor and destructor", "[class][ctor-dtor]") {
  SECTION("Class with both constructor and destructor") {
    REQUIRE(analyzeSource("class Person { Person() { } ~Person() { } }") ==
            true);
  }
}

TEST_CASE("Class: Single inheritance", "[class][inheritance]") {
  SECTION("Basic inheritance") {
    REQUIRE(analyzeSource("class Animal { } class Dog : Animal { }") == true);
  }

  SECTION("Inheritance with members") {
    REQUIRE(analyzeSource("class Animal { protected int age; } class Dog : "
                          "Animal { public int getAge() { return age; } }") ==
            true);
  }

  SECTION("Inheritance with methods") {
    REQUIRE(analyzeSource("class Animal { public void speak() { } } class Dog "
                          ": Animal { public void speak() { } }") == true);
  }

  SECTION("Derived class can access protected members") {
    REQUIRE(analyzeSource("class Base { protected int x; } class Derived : "
                          "Base { public int getX() { return x; } }") == true);
  }

  SECTION("Derived class cannot access private members") {
    REQUIRE(analyzeSource("class Base { private int x; } class Derived : Base "
                          "{ public int getX() { return x; } }") == false);
  }
}

TEST_CASE("Class: Multiple inheritance", "[class][inheritance][multiple]") {
  SECTION("Class inherits from multiple classes") {
    REQUIRE(analyzeSource("class Flyable { } class Swimmable { } class Duck : "
                          "Flyable, Swimmable { }") == true);
  }

  SECTION("Multiple inheritance with members") {
    REQUIRE(analyzeSource(
                "class Flyable { protected int wingSpan; } class Swimmable { "
                "protected int finCount; } class Duck : Flyable, Swimmable { "
                "public int getTotal() { return wingSpan + finCount; } }") ==
            true);
  }
}

TEST_CASE("Class: Polymorphism", "[class][polymorphism]") {
  SECTION("Base class pointer can point to derived class") {
    REQUIRE(analyzeSource("class Animal { } class Dog : Animal { } func test() "
                          "{ Dog dog; Animal^ animal = ^dog; }") == true);
  }

  SECTION("Virtual method dispatch") {
    REQUIRE(analyzeSource("class Animal { public virtual void speak() { } } "
                          "class Dog : Animal { public void speak() { } }") ==
            true);
  }

  SECTION("Abstract class") {
    REQUIRE(analyzeSource(
                "abstract class Animal { public abstract void speak(); } class "
                "Dog : Animal { public void speak() { } }") == true);
  }

  SECTION("Cannot instantiate abstract class") {
    REQUIRE(analyzeSource("abstract class Animal { public abstract void "
                          "speak(); } func test() { Animal animal; }") ==
            false);
  }
}

TEST_CASE("Class: super keyword", "[class][inheritance][super]") {
  SECTION("Call base class constructor") {
    REQUIRE(analyzeSource("class Animal { public Animal(int age) { } } class "
                          "Dog : Animal { public Dog() : super(0) { } }") ==
            true);
  }

  SECTION("Call base class method") {
    REQUIRE(analyzeSource(
                "class Animal { public void speak() { } } class Dog : Animal { "
                "public void speak() { super.speak(); } }") == true);
  }
}

TEST_CASE("Class: Interface inheritance", "[class][interface]") {
  SECTION("Class implements interface") {
    REQUIRE(analyzeSource("interface Drawable { void draw(); } class Circle : "
                          "Drawable { public void draw() { } }") == true);
  }

  SECTION("Class implements multiple interfaces") {
    REQUIRE(analyzeSource(
                "interface Drawable { void draw(); } interface Resizable { "
                "void resize(); } class Shape : Drawable, Resizable { public "
                "void draw() { } public void resize() { } }") == true);
  }

  SECTION("Class must implement all interface methods") {
    REQUIRE(analyzeSource("interface Drawable { void draw(); } class Circle : "
                          "Drawable { }") == false);
  }
}

TEST_CASE("Class: Complex inheritance scenarios",
          "[class][inheritance][complex]") {
  SECTION("Diamond inheritance") {
    REQUIRE(analyzeSource(
                "class Animal { protected int age; } class Mammal : Animal { } "
                "class Bird : Animal { } class Bat : Mammal, Bird { }") ==
            true);
  }

  SECTION("Deep inheritance chain") {
    REQUIRE(analyzeSource("class A { } class B : A { } class C : B { } class D "
                          ": C { } func test() { D d; }") == true);
  }

  SECTION("Inheritance with property override") {
    REQUIRE(analyzeSource(
                "class Shape { protected int x; protected int y; public int "
                "getX() { return x; } } class Circle : Shape { protected int "
                "radius; public int getX() { return x + radius; } }") == true);
  }
}
