#include "ComptimeStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ComptimeStmt::toString() const {
  return std::format("ComptimeStmt({})", stmt->toString());
}

} // namespace ast
} // namespace c_hat
