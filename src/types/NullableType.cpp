#include "NullableType.h"
#include "TypeFactory.h"
#include <format>

namespace c_hat {
namespace types {

NullableType::NullableType(std::shared_ptr<Type> baseType)
    : baseType(std::move(baseType)) {}

std::string NullableType::toString() const {
  return baseType ? baseType->toString() + "?" : "?";
}

bool NullableType::isCompatibleWithImpl(const Type &other) const {
  if (auto *otherNullable = dynamic_cast<const NullableType *>(&other)) {
    if (!baseType || !otherNullable->baseType) {
      return false;
    }
    return baseType->isCompatibleWith(*otherNullable->baseType);
  }
  return false;
}

bool NullableType::isSubtypeOfImpl(const Type &other) const {
  // T? 是 T? 的子类型
  if (auto *otherNullable = dynamic_cast<const NullableType *>(&other)) {
    if (!baseType || !otherNullable->baseType) {
      return false;
    }
    return baseType->isSubtypeOf(*otherNullable->baseType);
  }
  // T 是 T? 的子类型（非空可以隐式转换为可空）
  if (baseType && baseType->isCompatibleWith(other)) {
    return true;
  }
  return false;
}

} // namespace types
} // namespace c_hat
