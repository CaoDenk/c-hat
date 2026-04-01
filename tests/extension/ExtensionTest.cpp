#include "../../src/lexer/Lexer.h"
#include "../../src/parser/Parser.h"
#include "../../src/semantic/ExtensionRegistry.h"
#include "../../src/semantic/SemanticAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <memory>
#include <string>

using namespace c_hat;

TEST_CASE("Extension: Parse extension declaration", "[extension]") {
  std::string code = R"(extension int {
  func twice() -> int {
    return self * 2;
  }
})";

  parser::Parser parser(code);

  auto program = parser.parseProgram();
  REQUIRE(program != nullptr);
  REQUIRE(program->declarations.size() == 1);

  auto *extDecl =
      dynamic_cast<ast::ExtensionDecl *>(program->declarations[0].get());
  REQUIRE(extDecl != nullptr);
  REQUIRE(extDecl->members.size() == 1);
}

TEST_CASE("Extension: ExtensionRegistry basic operations", "[extension]") {
  semantic::ExtensionRegistry registry;

  auto intType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int);

  ast::ExtensionDecl extDecl(nullptr, {});

  registry.addExtension(intType, &extDecl);

  auto extensions = registry.getExtensionsForType(intType);
  REQUIRE(extensions.size() == 1);
  REQUIRE(extensions[0] == &extDecl);
}
