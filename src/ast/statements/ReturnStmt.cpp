#include "ReturnStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ReturnStmt::toString() const {
  if (expr) {
    return std::format("ReturnStmt({})", expr->toString());
  }
  return "ReturnStmt()";
}

} // namespace ast
} // namespace c_hat
