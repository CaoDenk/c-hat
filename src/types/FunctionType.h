#pragma once

#include "Type.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace types {

// 函数类型
class FunctionType : public Type {
public:
  // 构造函数
  FunctionType(std::shared_ptr<Type> returnType,
               std::vector<std::shared_ptr<Type>> parameterTypes);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为函数类型
  bool isFunction() const override { return true; }

  // 获取返回类型
  std::shared_ptr<Type> getReturnType() const { return returnType; }

  // 获取参数类型列表
  const std::vector<std::shared_ptr<Type>> &getParameterTypes() const {
    return parameterTypes;
  }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<Type>> parameterTypes;
};

} // namespace types
} // namespace c_hat
