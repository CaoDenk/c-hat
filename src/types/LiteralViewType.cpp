#include "LiteralViewType.h"
#include "PrimitiveType.h"
#include "ReadonlyType.h"
#include "SliceType.h"
#include "TypeFactory.h"

namespace c_hat {
namespace types {

std::string LiteralViewType::toString() const { return "LiteralView"; }

bool LiteralViewType::isCompatibleWithImpl(const Type &other) const {
  if (this == &other) {
    return true;
  }

  // 与其他 LiteralView 兼容
  if (other.isLiteralView()) {
    return true;
  }

  // 与 byte^（byte 指针）兼容
  if (other.isPointer()) {
    const auto *pointerType = static_cast<const PointerType *>(&other);
    const auto &pointeeType = pointerType->getPointeeType();
    if (pointeeType->isPrimitive()) {
      const auto *primType =
          static_cast<const PrimitiveType *>(pointeeType.get());
      if (primType->getKind() == PrimitiveType::Kind::Byte) {
        return true;
      }
    }
  }

  // 与 byte[]（Slice<byte>）兼容，当被 Readonly 包装时（byte![]）
  // 注意：Type::isCompatibleWith 会先处理顶层 Readonly 的兼容性，
  // 所以这里只要检查 Slice<byte> 即可，Readonly 的处理在基类中
  if (other.isSlice()) {
    const auto *sliceType = static_cast<const SliceType *>(&other);
    const auto &elemType = sliceType->getElementType();
    if (elemType->isPrimitive()) {
      const auto *primType = static_cast<const PrimitiveType *>(elemType.get());
      if (primType->getKind() == PrimitiveType::Kind::Byte) {
        return true;
      }
    }
  }

  return false;
}

bool LiteralViewType::isSubtypeOfImpl(const Type &other) const {
  // LiteralView 是 byte![] 的子类型
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
