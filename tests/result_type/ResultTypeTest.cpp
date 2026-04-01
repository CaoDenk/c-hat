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

TEST_CASE("Result: Result type declaration", "[result][declaration]") {
  SECTION("Result with int and string") {
    REQUIRE(analyzeSource("class Result<T, E> { } func test() -> Result<int, "
                          "string> { return Result<int, string>(); }") == true);
  }

  SECTION("Result with custom error type") {
    REQUIRE(analyzeSource("class Result<T, E> { } "
                          "struct MyError { int code; } "
                          "func test() -> Result<int, MyError> { return "
                          "Result<int, MyError>(); }") == true);
  }
}

TEST_CASE("Result: Result with ? operator", "[result][question_mark]") {
  SECTION("Function returning Result with ? propagation") {
    REQUIRE(
        analyzeSource(
            "class Result<T, E> { } "
            "func parse(string s) -> Result<int, string> { return Result<int, "
            "string>(); } "
            "func process(string s) -> Result<int, string> { int val = "
            "parse(s)?; return val; }") == true);
  }
}

TEST_CASE("Result: Optional type", "[result][optional]") {
  SECTION("Optional with int") {
    REQUIRE(analyzeSource("class Optional<T> { } func find() -> Optional<int> "
                          "{ return Optional<int>(); }") == true);
  }

  SECTION("Optional with class type") {
    REQUIRE(analyzeSource("class Optional<T> { } "
                          "class User { } func findUser(int id) -> "
                          "Optional<User> { return Optional<User>(); }") ==
            true);
  }
}

TEST_CASE("Result: Error propagation pattern", "[result][propagation]") {
  SECTION("Multiple ? in chain") {
    REQUIRE(
        analyzeSource(
            "class Result<T, E> { } "
            "func open(literalview path) -> Result<int, string> { return "
            "Result<int, string>(); } "
            "func read(int fd) -> Result<int, string> { return Result<int, "
            "string>(); } "
            "func parse(int data) -> Result<int, string> { return Result<int, "
            "string>(); } "
            "func workflow() -> Result<int, string> { "
            "  int fd = open(\"file\")?; "
            "  int data = read(fd)?; "
            "  return parse(data); }") == true);
  }
}
