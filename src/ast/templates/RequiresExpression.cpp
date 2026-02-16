#include "RequiresExpression.h"
#include <format>

namespace c_hat {
namespace ast {

std::string RequiresExpression::toString() const {
  std::string paramsStr = "";
  for (size_t i = 0; i < params.size(); i++) {
    if (i > 0)
      paramsStr += ", ";
    paramsStr += params[i]->toString();
  }
  std::string reqsStr = "";
  for (size_t i = 0; i < requirements.size(); i++) {
    if (i > 0)
      reqsStr += ", ";
    reqsStr += requirements[i]->toString();
  }
  return std::format("RequiresExpression(({}), [{}])", paramsStr, reqsStr);
}

} // namespace ast
} // namespace c_hat
