#include "DeferStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string DeferStmt::toString() const {
  if (body) {
    return std::format("DeferStmt({})", body->toString());
  }
  return std::format("DeferStmt({})", expr->toString());
}

} // namespace ast
} // namespace c_hat
