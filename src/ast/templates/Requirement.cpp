#include "Requirement.h"
#include <format>

namespace c_hat {
namespace ast {

std::string Requirement::toString() const {
  std::string typeStr;
  switch (type) {
  case Requirement::RequirementType::Compound:
    typeStr = "Compound";
    break;
  case Requirement::RequirementType::Typename:
    typeStr = "Typename";
    break;
  default:
    typeStr = "Unknown";
    break;
  }
  if (type == Requirement::RequirementType::Compound && expr) {
    return std::format("Requirement({}, {{{}}}, {})", typeStr, expr->toString(),
                       typeSpec->toString());
  } else {
    return std::format("Requirement({}, {})", typeStr, typeSpec->toString());
  }
}

} // namespace ast
} // namespace c_hat
