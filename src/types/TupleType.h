#pragma once

#include "Type.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace types {

// 元组类型
class TupleType : public Type {
public:
  // 构造函数
  explicit TupleType(std::vector<std::shared_ptr<Type>> elementTypes);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为元组类型
  bool isTuple() const override { return true; }

  // 获取元素类型列表
  const std::vector<std::shared_ptr<Type>> &getElementTypes() const {
    return elementTypes;
  }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::vector<std::shared_ptr<Type>> elementTypes;
};

} // namespace types
} // namespace c_hat
