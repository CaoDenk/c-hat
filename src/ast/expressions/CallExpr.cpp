#include "CallExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string CallExpr::toString() const {
  std::string argsStr = "";
  for (size_t i = 0; i < args.size(); i++) {
    if (i > 0)
      argsStr += ", ";
    argsStr += args[i]->toString();
  }
  return std::format("CallExpr({}, [{}])", callee->toString(), argsStr);
}

} // namespace ast
} // namespace c_hat
