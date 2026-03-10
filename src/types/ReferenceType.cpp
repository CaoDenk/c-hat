#include "ReferenceType.h"
#include <format>

namespace c_hat {
namespace types {

ReferenceType::ReferenceType(std::shared_ptr<Type> baseType)
    : baseType(baseType) {}

std::string ReferenceType::toString() const {
  return std::format("ref {}", baseType->toString());
}

bool ReferenceType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isReference()) {
    return false;
  }
  const auto &otherRef = static_cast<const ReferenceType &>(other);
  return baseType->isCompatibleWith(*otherRef.baseType);
}

bool ReferenceType::isSubtypeOfImpl(const Type &other) const {
  if (other.isReference()) {
    const auto &otherRef = static_cast<const ReferenceType &>(other);
    return baseType->isSubtypeOf(*otherRef.baseType);
  }
  return false;
}

} // namespace types
} // namespace c_hat