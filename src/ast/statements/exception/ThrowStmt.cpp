#include "ThrowStmt.h"

namespace c_hat {
namespace ast {

std::string ThrowStmt::toString() const {
  if (expr) {
    return "ThrowStmt(" + expr->toString() + ")";
  }
  return "ThrowStmt()";
}

} // namespace ast
} // namespace c_hat
