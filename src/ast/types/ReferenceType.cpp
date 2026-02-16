#include "ReferenceType.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ReferenceType::toString() const {
  return std::format("ReferenceType({})", baseType->toString());
}

} // namespace ast
} // namespace c_hat
