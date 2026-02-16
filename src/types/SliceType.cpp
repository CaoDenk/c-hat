#include "SliceType.h"
#include <format>

namespace c_hat {
namespace types {

SliceType::SliceType(std::shared_ptr<Type> elementType)
    : elementType(elementType) {}

std::string SliceType::toString() const {
  return std::format("{}[]", elementType->toString());
}

bool SliceType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isSlice()) {
    return false;
  }
  const auto &otherSlice = static_cast<const SliceType &>(other);
  return elementType->isCompatibleWith(*otherSlice.elementType);
}

bool SliceType::isSubtypeOfImpl(const Type &other) const {
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
