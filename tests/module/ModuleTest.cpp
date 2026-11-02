// ModuleTest.cpp - 模块系统设计 (docs/design/模块系统设计.md)
#include "../src/parser/Parser.h"
#include "../src/semantic/SemanticAnalyzer.h"
#include "../src/semantic/ModuleLoader.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
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

TEST_CASE("Module: duplicate top-level symbols are rejected", "[module][errors]") {
  namespace fs = std::filesystem;
  fs::path dir = fs::temp_directory_path() / "c_hat_module_dup_test";
  fs::remove_all(dir);
  fs::create_directories(dir);
  {
    std::ofstream a(dir / "A.ch");
    a << "module A;\npublic func foo() -> int { return 1; }\n";
  }
  {
    std::ofstream b(dir / "B.ch");
    b << "module B;\npublic func foo() -> int { return 2; }\n";
  }

  parser::Parser parser(
      "import A; import B; func main() { A.foo(); B.foo(); }");
  auto program = parser.parseProgram();
  REQUIRE(program != nullptr);
  semantic::ModuleLoader loader(std::vector<std::string>{dir.string()});
  REQUIRE_NOTHROW(semantic::loadImportsInto(*program, loader));
  REQUIRE(loader.getFunctionAliases().empty());

  semantic::SemanticAnalyzer analyzer(std::vector<std::string>{dir.string()},
                                      false);
  analyzer.setModuleFunctionAliases(loader.getFunctionAliases());
  analyzer.setModuleTypeAliases(loader.getTypeAliases());
  analyzer.analyze(*program);
  REQUIRE(!analyzer.hasError());

  {
    std::ofstream d(dir / "D.ch");
    d << "module D;\npublic struct P { int x; }\n";
  }
  {
    std::ofstream e(dir / "E.ch");
    e << "module E;\npublic struct P { int y; }\n";
  }
  parser::Parser dupTypeParser("import D; import E; func main() { }");
  auto dupTypeProgram = dupTypeParser.parseProgram();
  REQUIRE(dupTypeProgram != nullptr);
  semantic::ModuleLoader dupTypeLoader(std::vector<std::string>{dir.string()});
  REQUIRE_THROWS_AS(
      semantic::loadImportsInto(*dupTypeProgram, dupTypeLoader),
      std::runtime_error);

  fs::remove_all(dir);
}
