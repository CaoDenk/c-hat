#include "ExprStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ExprStmt::toString() const {
  return std::format("ExprStmt({})", expr->toString());
}

} // namespace ast
} // namespace c_hat
