#pragma once

#include "Type.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace types {

// 函数类型
class FunctionType : public Type {
public:
  FunctionType() = default;
  
  FunctionType(std::shared_ptr<Type> returnType,
               std::vector<std::shared_ptr<Type>> parameterTypes);

  std::string toString() const override;

  bool isFunction() const override { return true; }

  std::shared_ptr<Type> getReturnType() const { return returnType; }
  
  void setReturnType(std::shared_ptr<Type> type) { returnType = type; }

  const std::vector<std::shared_ptr<Type>> &getParameterTypes() const {
    return parameterTypes;
  }
  
  void addParameterType(std::shared_ptr<Type> type) {
    parameterTypes.push_back(type);
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
