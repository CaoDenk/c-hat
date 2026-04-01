#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

static std::unique_ptr<c_hat::ast::Program>
parseSource(const std::string &source) {
  try {
    c_hat::parser::Parser parser(source);
    return parser.parseProgram();
  } catch (...) {
    return nullptr;
  }
}

static bool analyzeSource(const std::string &source) {
  try {
    c_hat::parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      return false;
    }

    c_hat::semantic::SemanticAnalyzer analyzer("", false);
    analyzer.analyze(*program);
    return !analyzer.hasError();
  } catch (...) {
    return false;
  }
}

TEST_CASE("Attribute: Parse attribute declaration", "[attribute][parser]") {
  SECTION("Empty attribute") {
    auto program = parseSource("attribute Empty {}");
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
    auto *attrDecl = dynamic_cast<c_hat::ast::AttributeDecl *>(
        program->declarations[0].get());
    REQUIRE(attrDecl != nullptr);
    REQUIRE(attrDecl->name == "Empty");
    REQUIRE(attrDecl->fields.empty());
  }

  SECTION("Attribute with fields") {
    auto program = parseSource(
        "attribute Column { name: literalview, nullable: bool = false }");
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 1);
    auto *attrDecl = dynamic_cast<c_hat::ast::AttributeDecl *>(
        program->declarations[0].get());
    REQUIRE(attrDecl != nullptr);
    REQUIRE(attrDecl->name == "Column");
    REQUIRE(attrDecl->fields.size() == 2);
    REQUIRE(attrDecl->fields[0]->name == "name");
    REQUIRE(attrDecl->fields[1]->name == "nullable");
  }
}

TEST_CASE("Attribute: Parse attribute application", "[attribute][parser]") {
  SECTION("Simple attribute on function") {
    auto program = parseSource(R"(
      attribute Inline {}
      
      [Inline]
      func foo() -> int {
        return 42;
      }
    )");
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 2);

    auto *funcDecl = dynamic_cast<c_hat::ast::FunctionDecl *>(
        program->declarations[1].get());
    REQUIRE(funcDecl != nullptr);
    REQUIRE(funcDecl->name == "foo");
    REQUIRE(funcDecl->attributes.size() == 1);
    REQUIRE(funcDecl->attributes[0]->name == "Inline");
  }

  SECTION("Attribute with arguments") {
    auto program = parseSource(R"(
      attribute JsonProperty { name: literalview }
      
      [JsonProperty("age")]
      int age;
    )");
    REQUIRE(program != nullptr);
    REQUIRE(program->declarations.size() == 2);

    auto *varDecl = dynamic_cast<c_hat::ast::VariableDecl *>(
        program->declarations[1].get());
    REQUIRE(varDecl != nullptr);
    REQUIRE(varDecl->name == "age");
    REQUIRE(varDecl->attributes.size() == 1);
    REQUIRE(varDecl->attributes[0]->name == "JsonProperty");
    REQUIRE(varDecl->attributes[0]->arguments.size() == 1);
  }

  SECTION("Multiple attributes") {
    auto program = parseSource(R"(
      attribute Inline {}
      attribute Export {}
      
      [Inline]
      [Export]
      func bar() {}
    )");
    REQUIRE(program != nullptr);

    auto *funcDecl = dynamic_cast<c_hat::ast::FunctionDecl *>(
        program->declarations[2].get());
    REQUIRE(funcDecl != nullptr);
    REQUIRE(funcDecl->attributes.size() == 2);
    REQUIRE(funcDecl->attributes[0]->name == "Inline");
    REQUIRE(funcDecl->attributes[1]->name == "Export");
  }

  SECTION("Attribute with named arguments") {
    auto program = parseSource(R"(
      attribute Column { name: literalview, nullable: bool }
      
      [Column(name = "user_id", nullable = true)]
      int userId;
    )");
    REQUIRE(program != nullptr);

    auto *varDecl = dynamic_cast<c_hat::ast::VariableDecl *>(
        program->declarations[1].get());
    REQUIRE(varDecl != nullptr);
    REQUIRE(varDecl->attributes.size() == 1);
    REQUIRE(varDecl->attributes[0]->name == "Column");
    REQUIRE(varDecl->attributes[0]->arguments.size() == 2);
    REQUIRE(varDecl->attributes[0]->arguments[0]->name == "name");
    REQUIRE(varDecl->attributes[0]->arguments[1]->name == "nullable");
  }
}

TEST_CASE("Attribute: Semantic analysis", "[attribute][semantic]") {
  SECTION("Attribute declaration semantic check") {
    REQUIRE(analyzeSource("attribute Serializable {}") == true);
  }

  SECTION("Attribute with type fields") {
    REQUIRE(analyzeSource(R"(
      attribute Column { 
        name: literalview, 
        nullable: bool = false 
      }
    )") == true);
  }

  SECTION("Attribute on function") {
    REQUIRE(analyzeSource(R"(
      attribute Inline {}
      
      [Inline]
      func foo() -> int {
        return 1;
      }
    )") == true);
  }
}
