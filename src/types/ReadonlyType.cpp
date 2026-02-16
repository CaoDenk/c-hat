#include "ReadonlyType.h"

namespace c_hat {
namespace types {

ReadonlyType::ReadonlyType(std::shared_ptr<Type> baseType)
    : baseType(std::move(baseType)) {}

std::string ReadonlyType::toString() const {
  return baseType->toString() + "!";
}

bool ReadonlyType::isCompatibleWithImpl(const Type &other) const {
  // 基类已经处理了 Readonly 包装，这里直接调用基类型的 isCompatibleWith
  return baseType->isCompatibleWith(other);
}

bool ReadonlyType::isSubtypeOfImpl(const Type &other) const {
  // 基类已经处理了 Readonly 包装，这里直接调用基类型的 isSubtypeOf
  return baseType->isSubtypeOf(other);
}

} // namespace types
} // namespace c_hat
