#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace types {

// 指针类型
class PointerType : public Type {
public:
  // 构造函数
  explicit PointerType(std::shared_ptr<Type> pointeeType,
                       bool nullable = false);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为指针类型
  bool isPointer() const override { return true; }

  // 获取指向的类型
  std::shared_ptr<Type> getPointeeType() const { return pointeeType; }

  // 检查是否为可空指针
  bool isNullable() const { return nullable; }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> pointeeType;
  bool nullable;
};

} // namespace types
} // namespace c_hat
