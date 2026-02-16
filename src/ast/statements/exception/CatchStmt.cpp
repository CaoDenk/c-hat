#include "CatchStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string CatchStmt::toString() const {
  std::string paramStr = "";
  if (param) {
    paramStr = param->toString();
  }
  return std::format("CatchStmt({}, {})", paramStr, body->toString());
}

} // namespace ast
} // namespace c_hat
