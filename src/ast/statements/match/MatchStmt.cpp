#include "MatchStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string MatchStmt::toString() const {
  std::string armsStr = "";
  for (size_t i = 0; i < arms.size(); i++) {
    if (i > 0)
      armsStr += ", ";
    armsStr += arms[i]->toString();
  }
  return std::format("MatchStmt({}, [{}])", expr->toString(), armsStr);
}

} // namespace ast
} // namespace c_hat
