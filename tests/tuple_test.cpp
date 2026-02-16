#include <iostream>
#include <string>

#include "codegen/CodeGenerator.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"

int main() {
  std::string source = R"(
func test() {
    var t1 = (1, "hello");
    var t2 = (3.14, true, 42);
}
)";

  std::cout << "Testing tuple expression semantic analysis:" << std::endl;
  std::cout << "Source:" << std::endl;
  std::cout << source << std::endl;

  try {
    // Parsing
    c_hat::parser::Parser parser(source);
    auto program = parser.parseProgram();
    std::cout << "✓ Parsing successful!" << std::endl;
    std::cout << "AST:" << std::endl;
    std::cout << program->toString() << std::endl;

    // Semantic analysis
    c_hat::semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(std::move(program));
    std::cout << "✓ Semantic analysis successful!" << std::endl;

    // Parse again for code generation
    c_hat::parser::Parser parser2(source);
    auto program2 = parser2.parseProgram();

    // Code generation
    c_hat::codegen::CodeGenerator codeGen;
    std::string generatedCode = codeGen.generate(std::move(program2));
    std::cout << "✓ Code generation successful!" << std::endl;
    std::cout << "Generated code:" << std::endl;
    std::cout << generatedCode << std::endl;

  } catch (const std::exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
  }

  return 0;
}
