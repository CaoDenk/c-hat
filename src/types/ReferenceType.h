#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace types {

// 引用类型
class ReferenceType : public Type {
public:
  // 构造函数
  explicit ReferenceType(std::shared_ptr<Type> baseType);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为引用类型
  bool isReference() const override { return true; }

  // 获取基础类型
  std::shared_ptr<Type> getBaseType() const { return baseType; }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> baseType;
};

} // namespace types
} // namespace c_hat