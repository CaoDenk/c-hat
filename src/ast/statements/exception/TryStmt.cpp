#include "TryStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string TryStmt::toString() const {
  std::string catchStr = "";
  for (size_t i = 0; i < catchStmts.size(); i++) {
    if (i > 0) {
      catchStr += ", ";
    }
    catchStr += catchStmts[i]->toString();
  }
  return std::format("TryStmt({}, [{}])", tryBlock->toString(), catchStr);
}

} // namespace ast
} // namespace c_hat
