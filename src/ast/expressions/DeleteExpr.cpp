#include "DeleteExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string DeleteExpr::toString() const {
  return std::format("DeleteExpr({}, {})", expr->toString(),
                     isArray ? "true" : "false");
}

} // namespace ast
} // namespace c_hat
