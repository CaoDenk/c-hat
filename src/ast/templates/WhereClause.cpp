#include "WhereClause.h"
#include <format>

namespace c_hat {
namespace ast {

std::string WhereClause::toString() const {
  std::string constraintsStr = "";
  for (size_t i = 0; i < constraints.size(); i++) {
    if (i > 0) {
      constraintsStr += ", ";
    }
    constraintsStr += std::format("{}: {}", constraints[i].typeParam,
                                  constraints[i].constraint->toString());
  }
  return std::format("WhereClause([{}])", constraintsStr);
}

} // namespace ast
} // namespace c_hat
