#include "Program.h"
#include <format>
#include "../declarations/Declaration.h"

namespace c_hat {
namespace ast {

std::string Program::toString() const {
  std::string declsStr = "";
  for (size_t i = 0; i < declarations.size(); i++) {
    if (i > 0)
      declsStr += ", \n";
    declsStr += declarations[i]->toString();
  }
  return std::format("Program([\n{}])", declsStr);
}

} // namespace ast
} // namespace c_hat
