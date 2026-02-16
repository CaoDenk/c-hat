#include "PointerType.h"
#include <format>

namespace c_hat {
namespace types {

PointerType::PointerType(std::shared_ptr<Type> pointeeType, bool nullable)
    : pointeeType(pointeeType), nullable(nullable) {}

std::string PointerType::toString() const {
  if (nullable) {
    return std::format("{}?^", pointeeType->toString());
  }
  return std::format("{}^", pointeeType->toString());
}

bool PointerType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isPointer()) {
    return false;
  }
  const auto &otherPointer = static_cast<const PointerType &>(other);

  if (nullable != otherPointer.isNullable()) {
    return false;
  }

  // 检查指针的兼容性：
  // 对于 T^ 赋值给 U^：
  // - 可以增加底层 const（T → T!）
  // - 不能去掉底层 const（T! → T）

  // 检查我们的 pointee 类型是否可以赋值给对方的 pointee 类型
  // 对方的 pointee 类型必须是我们的 pointee 类型或者我们的 pointee 类型的
  // Readonly 版本

  // 检查 other.pointeeType 是否是 this.pointeeType 的超类型（包括增加
  // Readonly） 即：this.pointeeType 可以赋值给 other.pointeeType
  return pointeeType->isSubtypeOf(*otherPointer.pointeeType);
}

bool PointerType::isSubtypeOfImpl(const Type &other) const {
  if (!other.isPointer()) {
    return false;
  }
  const auto &otherPointer = static_cast<const PointerType &>(other);
  return pointeeType->isSubtypeOf(*otherPointer.pointeeType) &&
         (!nullable || otherPointer.isNullable());
}

} // namespace types
} // namespace c_hat
