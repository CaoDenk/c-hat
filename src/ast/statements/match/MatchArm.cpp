#include "MatchArm.h"
#include <format>

namespace c_hat {
namespace ast {

std::string MatchArm::toString() const {
  std::string guardStr = "";
  if (guard) {
    guardStr = std::format(" if {}", guard->toString());
  }
  return std::format("MatchArm({}, {}{})", pattern->toString(),
                     body->toString(), guardStr);
}

} // namespace ast
} // namespace c_hat
