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
  std::string bodyStr = "";
  if (body) {
    bodyStr = std::format(", {}", body->toString());
  }
  if (arrowExpr) {
    bodyStr = std::format(", => {}", arrowExpr->toString());
  }
  return std::format("FunctionDecl({} {}({}){}{})", specifiers, name, paramsStr,
                     returnTypeStr, bodyStr);
}

} // namespace ast
} // namespace c_hat
