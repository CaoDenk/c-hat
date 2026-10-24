#include "LiteralViewType.h"
#include "PrimitiveType.h"
#include "ReadonlyType.h"
#include "SliceType.h"
#include "TypeFactory.h"

namespace c_hat {
namespace types {

std::string LiteralViewType::toString() const { return "literalview"; }

bool LiteralViewType::isCompatibleWithImpl(const Type &other) const {
  if (this == &other) {
    return true;
  }

  // 与其他 literalview 兼容
  if (other.isLiteralView()) {
    return true;
  }

  // 与只读 byte 指针（byte!^）兼容；可写 byte^ 不允许
  if (other.isPointer()) {
    const auto *pointerType = static_cast<const PointerType *>(&other);
    const auto &pointeeType = pointerType->getPointeeType();
    if (!pointeeType->isReadonly()) {
      return false;
    }
    auto plainPointee = unwrapReadonly(pointeeType);
    if (plainPointee && plainPointee->isPrimitive()) {
      const auto *primType =
          static_cast<const PrimitiveType *>(plainPointee.get());
      if (primType->getKind() == PrimitiveType::Kind::Byte) {
        return true;
      }
    }
  }

  // 与只读 byte 切片（byte![]）兼容；可写 byte[] 不允许
  if (other.isSlice()) {
    const auto *sliceType = static_cast<const SliceType *>(&other);
    const auto &elemType = sliceType->getElementType();
    if (!elemType->isReadonly()) {
      return false;
    }
    auto plainElem = unwrapReadonly(elemType);
    if (plainElem && plainElem->isPrimitive()) {
      const auto *primType = static_cast<const PrimitiveType *>(plainElem.get());
      if (primType->getKind() == PrimitiveType::Kind::Byte) {
        return true;
      }
    }
  }

  return false;
}

bool LiteralViewType::isSubtypeOfImpl(const Type &other) const {
  // literalview 是 byte![] 的子类型
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
