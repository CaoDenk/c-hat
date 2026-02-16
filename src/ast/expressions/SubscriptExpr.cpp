#include "SubscriptExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string SubscriptExpr::toString() const {
  return std::format("SubscriptExpr({}, {})", object->toString(),
                     index->toString());
}

} // namespace ast
} // namespace c_hat
