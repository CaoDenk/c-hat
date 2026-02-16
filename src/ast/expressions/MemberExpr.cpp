#include "MemberExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string MemberExpr::toString() const {
  return std::format("MemberExpr({}, {})", object->toString(), member);
}

} // namespace ast
} // namespace c_hat
