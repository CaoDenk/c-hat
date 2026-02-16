#include "VariableDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string VariableDecl::toString() const {
  std::string initStr = "";
  if (initializer) {
    initStr = std::format(", {}", initializer->toString());
  }
  std::string specStr = specifiers.empty() ? "" : specifiers + " ";
  std::string lateStr = isLate ? "late " : "";
  std::string constStr = isConst ? "const " : "";
  std::string typeStr = type ? type->toString() : "(inferred)";
  return std::format("VariableDecl({}{}{}{}, {}{})", specStr, lateStr, constStr, typeStr, name,
                     initStr);
}

} // namespace ast
} // namespace c_hat
