#include "RequiresClause.h"
#include <format>

namespace c_hat {
namespace ast {

std::string RequiresClause::toString() const {
  return std::format("RequiresClause({})", expr->toString());
}

} // namespace ast
} // namespace c_hat
