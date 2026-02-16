#include "RectangularArrayType.h"
#include <format>

namespace c_hat {
namespace types {

RectangularArrayType::RectangularArrayType(std::shared_ptr<Type> elementType,
                                           std::vector<size_t> sizes)
    : elementType(std::move(elementType)), sizes(std::move(sizes)) {}

std::string RectangularArrayType::toString() const {
  std::string result = elementType->toString() + "[";
  for (size_t i = 0; i < sizes.size(); ++i) {
    if (i > 0) {
      result += ",";
    }
    result += std::format("{}", sizes[i]);
  }
  result += "]";
  return result;
}

bool RectangularArrayType::isCompatibleWithImpl(const Type &other) const {
  if (auto otherRectArray =
          dynamic_cast<const RectangularArrayType *>(&other)) {
    if (sizes.size() != otherRectArray->sizes.size()) {
      return false;
    }
    for (size_t i = 0; i < sizes.size(); ++i) {
      if (sizes[i] != otherRectArray->sizes[i]) {
        return false;
      }
    }
    return elementType->isCompatibleWith(*otherRectArray->elementType);
  }
  return false;
}

bool RectangularArrayType::isSubtypeOfImpl(const Type &other) const {
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
