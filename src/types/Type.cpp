#include "Type.h"
#include "ReadonlyType.h"

namespace c_hat {
namespace types {

std::shared_ptr<const Type> unwrapReadonly(const Type *type) {
  if (!type)
    return nullptr;
  std::shared_ptr<const Type> current(type, [](const Type *) {});
  while (current && current->isReadonly()) {
    current = current->getBaseType();
  }
  return current;
}

std::shared_ptr<Type> unwrapReadonly(std::shared_ptr<Type> type) {
  if (!type)
    return nullptr;
  std::shared_ptr<Type> current = type;
  while (current && current->isReadonly()) {
    current = current->getBaseType();
  }
  return current;
}

bool Type::isCompatibleWith(const Type &other) const {
  if (this == &other)
    return true;

  // 处理顶层 Readonly：可以双向兼容
  // Case 1: 我是 Readonly，对方不是 → 我可以兼容对方（值拷贝去掉顶层 const）
  if (this->isReadonly()) {
    const auto *readonlyThis = static_cast<const ReadonlyType *>(this);
    if (readonlyThis->getBaseType()->isCompatibleWith(other)) {
      return true;
    }
  }

  // Case 2: 对方是 Readonly，我不是 → 我可以兼容对方（值拷贝增加顶层 const）
  if (other.isReadonly()) {
    const auto *readonlyOther = static_cast<const ReadonlyType *>(&other);
    if (this->isCompatibleWith(*readonlyOther->getBaseType())) {
      return true;
    }
  }

  // Case 3: 双方都是 Readonly → 检查 baseType 的兼容性
  if (this->isReadonly() && other.isReadonly()) {
    const auto *readonlyThis = static_cast<const ReadonlyType *>(this);
    const auto *readonlyOther = static_cast<const ReadonlyType *>(&other);
    if (readonlyThis->getBaseType()->isCompatibleWith(
            *readonlyOther->getBaseType())) {
      return true;
    }
  }

  // Case 4: 双方都不是 Readonly → 直接调用具体类型的实现
  if (!this->isReadonly() && !other.isReadonly()) {
    return this->isCompatibleWithImpl(other);
  }

  return false;
}

bool Type::isSubtypeOf(const Type &other) const {
  if (this == &other)
    return true;

  // 处理顶层 Readonly：可以双向兼容
  // Case 1: 我是 Readonly，对方不是 → 我是对方的子类型
  if (this->isReadonly()) {
    const auto *readonlyThis = static_cast<const ReadonlyType *>(this);
    if (readonlyThis->getBaseType()->isSubtypeOf(other)) {
      return true;
    }
  }

  // Case 2: 对方是 Readonly，我不是 → 我是对方的子类型
  if (other.isReadonly()) {
    const auto *readonlyOther = static_cast<const ReadonlyType *>(&other);
    if (this->isSubtypeOf(*readonlyOther->getBaseType())) {
      return true;
    }
  }

  // Case 3: 双方都是 Readonly → 检查 baseType 的子类型关系
  if (this->isReadonly() && other.isReadonly()) {
    const auto *readonlyThis = static_cast<const ReadonlyType *>(this);
    const auto *readonlyOther = static_cast<const ReadonlyType *>(&other);
    if (readonlyThis->getBaseType()->isSubtypeOf(
            *readonlyOther->getBaseType())) {
      return true;
    }
  }

  // Case 4: 双方都不是 Readonly → 直接调用具体类型的实现
  if (!this->isReadonly() && !other.isReadonly()) {
    return this->isSubtypeOfImpl(other);
  }

  return false;
}

} // namespace types
} // namespace c_hat
