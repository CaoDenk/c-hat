#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace types {

// 只读类型（如 byte!）
class ReadonlyType : public Type {
public:
  explicit ReadonlyType(std::shared_ptr<Type> baseType);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为只读类型
  bool isReadonly() const override { return true; }

  // 获取基础类型
  std::shared_ptr<Type> getBaseType() const override { return baseType; }

protected:
  // 具体类型的兼容性检查实现（基类已经处理 Readonly
  // 包装，这里直接调用基类型的实现）
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现（基类已经处理 Readonly
  // 包装，这里直接调用基类型的实现）
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> baseType;
};

} // namespace types
} // namespace c_hat
