#include "src/lexer/Lexer.h"
#include <iostream>


using namespace c_hat::lexer;

int main() {
  std::string source = "byte! b = 42;";
  Lexer lexer(source);

  std::cout << "Source: \"" << source << "\"\n\n";
  std::cout << "Tokens:\n";

  while (true) {
    auto token = lexer.nextToken();
    if (!token) {
      std::cout << "Error: No token\n";
      break;
    }

    std::cout << "  " << token->toString() << "\n";

    if (token->getType() == TokenType::EndOfFile) {
      break;
    }
  }

  return 0;
}
