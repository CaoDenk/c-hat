#include "WhileStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string WhileStmt::toString() const {
  return std::format("WhileStmt({}, {})", condition->toString(),
                     body->toString());
}

} // namespace ast
} // namespace c_hat
