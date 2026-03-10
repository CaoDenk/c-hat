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
  // 检查是否是兼容的切片类型
  if (other.isSlice()) {
    const auto &otherSlice = static_cast<const SliceType &>(other);
    return elementType->isSubtypeOf(*otherSlice.elementType);
  }
  
  // 检查是否是兼容的数组类型（切片可以转换为数组吗？不，数组可以转换为切片）
  return false;
}

} // namespace types
} // namespace c_hat
