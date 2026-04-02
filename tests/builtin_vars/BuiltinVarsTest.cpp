#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>

static bool parseSource(const std::string &source) {
  try {
    c_hat::parser::Parser parser(source);
    auto program = parser.parseProgram();
    return program != nullptr;
  } catch (...) {
    return false;
  }
}

static bool analyzeSource(const std::string &source) {
  try {
    c_hat::parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      std::cerr << "Parse failed" << std::endl;
      return false;
    }

    c_hat::semantic::SemanticAnalyzer analyzer("", false);
    analyzer.analyze(*program);
    if (analyzer.hasError()) {
      std::cerr << "Semantic analysis failed" << std::endl;
    }
    return !analyzer.hasError();
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cerr << "Unknown exception" << std::endl;
    return false;
  }
}

TEST_CASE("Builtin Variables: Parse builtin variables",
          "[builtin_vars][parse]") {
  SECTION("Parse __line__") {
    REQUIRE(parseSource("func main() { "
                        "  __line__; "
                        "}") == true);
  }

  SECTION("Parse __file__") {
    REQUIRE(parseSource("func main() { "
                        "  __file__; "
                        "}") == true);
  }

  SECTION("Parse __function__") {
    REQUIRE(parseSource("func main() { "
                        "  __function__; "
                        "}") == true);
  }

  SECTION("Parse __module__") {
    REQUIRE(parseSource("func main() { "
                        "  __module__; "
                        "}") == true);
  }

  SECTION("Parse __compiler_version__") {
    REQUIRE(parseSource("func main() { "
                        "  __compiler_version__; "
                        "}") == true);
  }

  SECTION("Parse __timestamp__") {
    REQUIRE(parseSource("func main() { "
                        "  __timestamp__; "
                        "}") == true);
  }

  SECTION("Parse __build_mode__") {
    REQUIRE(parseSource("func main() { "
                        "  __build_mode__; "
                        "}") == true);
  }

  SECTION("Parse __column__") {
    REQUIRE(parseSource("func main() { "
                        "  __column__; "
                        "}") == true);
  }
}

TEST_CASE("Builtin Variables: Semantic analysis", "[builtin_vars][semantic]") {
  SECTION("All builtin variables in one function") {
    REQUIRE(analyzeSource("func main() { "
                          "  __line__; "
                          "  __file__; "
                          "  __function__; "
                          "  __module__; "
                          "  __compiler_version__; "
                          "  __timestamp__; "
                          "  __build_mode__; "
                          "  __column__; "
                          "}") == true);
  }

  SECTION("Builtin variables in different scopes") {
    REQUIRE(analyzeSource("func test() { "
                          "  if (true) { "
                          "    __line__; "
                          "    __file__; "
                          "    __function__; "
                          "  } "
                          "  for (int i = 0; i < 5; i++) { "
                          "    __line__; "
                          "    __column__; "
                          "  } "
                          "}") == true);
  }

  SECTION("Builtin variables as function arguments") {
    REQUIRE(analyzeSource("func log(int line) { "
                          "  int x = line; "
                          "}"
                          "func main() { "
                          "  log(__line__); "
                          "}") == true);
  }
}
