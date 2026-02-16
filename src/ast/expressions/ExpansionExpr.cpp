#include "ExpansionExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ExpansionExpr::toString() const {
  return std::format("ExpansionExpr({})", expr->toString());
}

} // namespace ast
} // namespace c_hat
