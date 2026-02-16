#include "ArrayType.h"
#include "SliceType.h"
#include <format>

namespace c_hat {
namespace types {

ArrayType::ArrayType(std::shared_ptr<Type> elementType, size_t size)
    : elementType(elementType), size(size) {}

std::string ArrayType::toString() const {
  return std::format("{}[{}]", elementType->toString(), size);
}

bool ArrayType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isArray()) {
    return false;
  }
  const auto &otherArray = static_cast<const ArrayType &>(other);
  return size == otherArray.size &&
         elementType->isCompatibleWith(*otherArray.elementType);
}

bool ArrayType::isSubtypeOfImpl(const Type &other) const {
  if (other.isSlice()) {
    const auto &otherSlice = static_cast<const SliceType &>(other);
    return elementType->isSubtypeOf(*otherSlice.getElementType());
  }
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
