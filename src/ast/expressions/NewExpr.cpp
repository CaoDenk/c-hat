#include "NewExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string NewExpr::toString() const {
  std::string argsStr = "";
  for (size_t i = 0; i < args.size(); i++) {
    if (i > 0)
      argsStr += ", ";
    argsStr += args[i]->toString();
  }
  return std::format("NewExpr({}, [{}])", type->toString(), argsStr);
}

} // namespace ast
} // namespace c_hat
