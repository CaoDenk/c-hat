#pragma once

#include "Type.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace types {

// 泛型类型
class GenericType : public Type {
public:
  // 构造函数
  GenericType(std::string name,
              std::vector<std::shared_ptr<Type>> typeArguments);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为泛型类型
  bool isGeneric() const override { return true; }

  // 获取泛型名称
  const std::string &getName() const { return name; }

  // 获取类型参数列表
  const std::vector<std::shared_ptr<Type>> &getTypeArguments() const {
    return typeArguments;
  }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::string name;
  std::vector<std::shared_ptr<Type>> typeArguments;
};

} // namespace types
} // namespace c_hat
