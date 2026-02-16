#include "FunctionDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string FunctionDecl::toString() const {
  std::string paramsStr = "";
  for (size_t i = 0; i < params.size(); i++) {
    if (i > 0)
      paramsStr += ", ";
    paramsStr += params[i]->toString();
  }
  std::string returnTypeStr = "";
  if (returnType) {
    returnTypeStr = std::format(" -> {}", returnType->toString());
  }
  return std::format("FunctionDecl({} {}({}){})", specifiers, name, paramsStr,
                     returnTypeStr);
}

} // namespace ast
} // namespace c_hat
