#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

using namespace c_hat;

bool parseSource(const std::string &source) {
  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    return program != nullptr;
  } catch (...) {
    return false;
  }
}

bool analyzeSource(const std::string &source) {
  try {
    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program)
      return false;

    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(std::move(program));
    return true;
  } catch (...) {
    return false;
  }
}

TEST_CASE("New and Delete: Parser tests", "[new_delete][parser]") {
  SECTION("Basic new expression without arguments") {
    REQUIRE(parseSource(R"(
func test() {
  var p = new int;
}
)") == true);
  }

  SECTION("New expression with arguments") {
    REQUIRE(parseSource(R"(
func test() {
  var p = new int(42);
}
)") == true);
  }

  SECTION("New expression with multiple arguments") {
    REQUIRE(parseSource(R"(
class Person {
  var name;
  var age;
}

func test() {
  var p = new Person("Alice", 25);
}
)") == true);
  }

  SECTION("Basic delete expression") {
    REQUIRE(parseSource(R"(
func test() {
  var p = new int;
  delete p;
}
)") == true);
  }

  SECTION("Delete array expression") {
    REQUIRE(parseSource(R"(
func test() {
  var p = new int[10];
  delete[] p;
}
)") == true);
  }

  SECTION("New expression with pointer type") {
    REQUIRE(parseSource(R"(
func test() {
  var p = new int^;
}
)") == true);
  }

  SECTION("New and delete in complex context") {
    REQUIRE(parseSource(R"(
func test() {
  if (true) {
    var p = new int(100);
    delete p;
  }
}
)") == true);
  }
}

TEST_CASE("New and Delete: Semantic analysis tests", "[new_delete][semantic]") {
  SECTION("New expression returns pointer type") {
    REQUIRE(analyzeSource(R"(
func test() {
  var p = new int;
}
)") == true);
  }

  SECTION("New expression with arguments") {
    REQUIRE(analyzeSource(R"(
func test() {
  var p = new int(42);
}
)") == true);
  }

  SECTION("Delete expression accepts pointer type") {
    REQUIRE(analyzeSource(R"(
func test() {
  var p = new int;
  delete p;
}
)") == true);
  }

  SECTION("Delete[] expression accepts pointer type") {
    REQUIRE(analyzeSource(R"(
func test() {
  var p = new int[10];
  delete[] p;
}
)") == true);
  }

  SECTION("Multiple new and delete operations") {
    REQUIRE(analyzeSource(R"(
func test() {
  var p1 = new int(1);
  var p2 = new int(2);
  delete p1;
  delete p2;
}
)") == true);
  }

  SECTION("New class type") {
    REQUIRE(analyzeSource(R"(
class Person {
  var name;
  var age;
}

func test() {
  var p = new Person;
}
)") == true);
  }
}
