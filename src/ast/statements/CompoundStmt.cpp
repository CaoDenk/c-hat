#include "CompoundStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string CompoundStmt::toString() const {
  std::string stmtsStr = "";
  for (size_t i = 0; i < statements.size(); i++) {
    if (i > 0)
      stmtsStr += ", ";
    stmtsStr += statements[i]->toString();
  }
  return std::format("CompoundStmt([{}])", stmtsStr);
}

} // namespace ast
} // namespace c_hat
