#include "../src/ast/AstNodes.h"
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>

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
    bool hasError = analyzer.hasError();
    if (hasError) {
      std::cout << "Semantic analysis failed for source: " << source
                << std::endl;
    }
    return !hasError;
  } catch (const std::exception &e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cout << "Unknown exception" << std::endl;
    return false;
  }
}

TEST_CASE("Property: Basic property with getter", "[property]") {
  REQUIRE(analyzeSource("class Person {\n"
                        "  private string _name;\n"
                        "  \n"
                        "  public get name -> string {\n"
                        "    return _name;\n"
                        "  }\n"
                        "}\n") == true);
}

TEST_CASE("Property: Property with arrow expression getter", "[property]") {
  REQUIRE(analyzeSource("class Point {\n"
                        "  private int _x;\n"
                        "  private int _y;\n"
                        "  \n"
                        "  public get x -> int => _x;\n"
                        "  public get y -> int => _y;\n"
                        "}\n") == true);
}

TEST_CASE("Property: Read-only property", "[property]") {
  REQUIRE(analyzeSource("class Circle {\n"
                        "  private double _radius;\n"
                        "  \n"
                        "  public get radius -> double {\n"
                        "    return _radius;\n"
                        "  }\n"
                        "  \n"
                        "  public get area -> double {\n"
                        "    return 3.14159 * _radius * _radius;\n"
                        "  }\n"
                        "}\n") == true);
}

TEST_CASE("Property: Property without getter should error on read",
          "[property]") {
  REQUIRE(analyzeSource("class Counter {\n"
                        "  private var _count: int;\n"
                        "  \n"
                        "  public set count(value: int) {\n"
                        "    _count = value;\n"
                        "  }\n"
                        "}\n"
                        "\n"
                        "func test() {\n"
                        "  var counter = new Counter();\n"
                        "  var c = counter.count;\n"
                        "}\n") == false);
}

TEST_CASE("Property: Property without setter should error on write",
          "[property]") {
  REQUIRE(analyzeSource("class Counter {\n"
                        "  private var _count: int;\n"
                        "  \n"
                        "  public get count -> int {\n"
                        "    return _count;\n"
                        "  }\n"
                        "}\n"
                        "\n"
                        "func test() {\n"
                        "  var counter = new Counter();\n"
                        "  counter.count = 10;\n"
                        "}\n") == false);
}
