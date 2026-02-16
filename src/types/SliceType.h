#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace types {

// 切片类型
class SliceType : public Type {
public:
  // 构造函数
  explicit SliceType(std::shared_ptr<Type> elementType);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为切片类型
  bool isSlice() const override { return true; }

  // 获取元素类型
  std::shared_ptr<Type> getElementType() const { return elementType; }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> elementType;
};

} // namespace types
} // namespace c_hat
