#pragma once

#include "Type.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace types {

// 类类型
class ClassType : public Type {
public:
  // 构造函数
  explicit ClassType(std::string name);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为类类型
  bool isClass() const override { return true; }

  // 获取类名
  const std::string &getName() const { return name; }

  // 添加基类
  void addBaseClass(std::shared_ptr<ClassType> baseClass);

  // 获取基类列表
  const std::vector<std::shared_ptr<ClassType>> &getBaseClasses() const {
    return baseClasses;
  }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::string name;
  std::vector<std::shared_ptr<ClassType>> baseClasses;
};

} // namespace types
} // namespace c_hat
