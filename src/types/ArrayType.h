#pragma once

#include "Type.h"
#include <cstddef>
#include <memory>

namespace c_hat {
namespace types {

// 数组类型
class ArrayType : public Type {
public:
  // 构造函数
  ArrayType(std::shared_ptr<Type> elementType, size_t size);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为数组类型
  bool isArray() const override { return true; }

  // 获取元素类型
  std::shared_ptr<Type> getElementType() const { return elementType; }

  // 获取数组大小
  size_t getSize() const { return size; }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> elementType;
  size_t size;
};

} // namespace types
} // namespace c_hat
