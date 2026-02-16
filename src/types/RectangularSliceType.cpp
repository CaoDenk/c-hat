#include "RectangularSliceType.h"
#include <format>

namespace c_hat {
namespace types {

RectangularSliceType::RectangularSliceType(std::shared_ptr<Type> elementType,
                                           int rank)
    : elementType(std::move(elementType)), rank(rank) {}

std::string RectangularSliceType::toString() const {
  std::string result = elementType->toString() + "[";
  for (int i = 1; i < rank; ++i) {
    result += ",";
  }
  result += "]";
  return result;
}

bool RectangularSliceType::isCompatibleWithImpl(const Type &other) const {
  if (auto otherSlice = dynamic_cast<const RectangularSliceType *>(&other)) {
    return rank == otherSlice->rank &&
           elementType->isCompatibleWith(*otherSlice->elementType);
  }
  return false;
}

bool RectangularSliceType::isSubtypeOfImpl(const Type &other) const {
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
