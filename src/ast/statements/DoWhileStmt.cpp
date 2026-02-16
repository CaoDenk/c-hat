#include "DoWhileStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string DoWhileStmt::toString() const {
  return std::format("DoWhileStmt({}, {})", body->toString(),
                     condition->toString());
}

} // namespace ast
} // namespace c_hat
