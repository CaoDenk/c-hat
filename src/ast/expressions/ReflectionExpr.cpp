#include "ReflectionExpr.h"

namespace c_hat {
namespace ast {

std::string ReflectionExpr::toString() const {
  if (kind == TargetKind::Type) {
    return "@" + (type ? type->toString() : "<unknown>");
  } else {
    return "@typeof(" + (expression ? expression->toString() : "") + ")";
  }
}

std::unique_ptr<Expression> ReflectionExpr::clone() const {
  if (kind == TargetKind::Type) {
    return std::make_unique<ReflectionExpr>(type->clone());
  } else {
    return std::make_unique<ReflectionExpr>(expression->clone());
  }
}

} // namespace ast
} // namespace c_hat
