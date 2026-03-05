#include "../src/parser/Parser.h"
#include "../src/lexer/Lexer.h"
#include <iostream>
#include <string>

using namespace c_hat;

int main() {
  std::string source = "func print(fmt: *int, ...) { }";
  std::cout << "Testing: " << source << std::endl;

  try {
    lexer::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    std::cout << "Tokens:" << std::endl;
    for (const auto &tok : tokens) {
      std::cout << "  " << lexer::TokenTypeToString(tok->getType());
      if (!tok->getValue().empty()) {
        std::cout << " = " << tok->getValue();
      }
      std::cout << std::endl;
    }

    parser::Parser parser(source);
    auto program = parser.parseProgram();
    if (!program) {
      std::cout << "Parser failed!" << std::endl;
      return 1;
    }
    std::cout << "Parser succeeded!" << std::endl;
  } catch (const std::exception &e) {
    std::cout << "Exception: " << e.what() << std::endl;
  } catch (...) {
    std::cout << "Unknown exception!" << std::endl;
  }

  return 0;
}
