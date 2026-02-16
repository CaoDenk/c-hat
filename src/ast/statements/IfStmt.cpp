#include "IfStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string IfStmt::toString() const {
  if (elseBranch) {
    return std::format("IfStmt({}, {}, {})", condition->toString(),
                       thenBranch->toString(), elseBranch->toString());
  }
  return std::format("IfStmt({}, {})", condition->toString(),
                     thenBranch->toString());
}

} // namespace ast
} // namespace c_hat
