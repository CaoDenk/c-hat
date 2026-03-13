#pragma once

#include "Type.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace c_hat {
namespace types {

// 接口方法信息
struct InterfaceMethod {
  std::string name;
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<Type>> paramTypes;
  bool hasDefaultImplementation = false;          // 是否有默认实现
  AccessModifier access = AccessModifier::Public; // 默认公共访问

  InterfaceMethod() = default;
  InterfaceMethod(const std::string &name, std::shared_ptr<Type> returnType,
                  std::vector<std::shared_ptr<Type>> paramTypes,
                  bool hasDefault = false,
                  AccessModifier access = AccessModifier::Public)
      : name(name), returnType(returnType), paramTypes(paramTypes),
        hasDefaultImplementation(hasDefault), access(access) {}

  // 添加拷贝构造函数
  InterfaceMethod(const InterfaceMethod &) = default;
  InterfaceMethod &operator=(const InterfaceMethod &) = default;
};

// 接口类型
class InterfaceType : public Type {
public:
  explicit InterfaceType(std::string name);
  InterfaceType(std::string name, std::vector<std::string> typeParameters);
  std::string toString() const override;
  bool isInterface() const override { return true; }
  const std::string &getName() const { return name; }

  // 获取泛型参数
  const std::vector<std::string> &getTypeParameters() const {
    return typeParameters;
  }
  bool isGeneric() const { return !typeParameters.empty(); }

  // 实例化泛型接口
  std::shared_ptr<InterfaceType>
  instantiate(const std::vector<std::shared_ptr<Type>> &typeArguments) const;

  // 添加父接口
  void addBaseInterface(std::shared_ptr<InterfaceType> baseInterface);
  const std::vector<std::shared_ptr<InterfaceType>> &getBaseInterfaces() const {
    return baseInterfaces;
  }

  // 添加方法
  void addMethod(const InterfaceMethod &method);
  const std::unordered_map<std::string, InterfaceMethod> &getMethods() const {
    return methods;
  }

  // 查找方法（包括继承的方法）
  bool hasMethod(const std::string &methodName) const;
  const InterfaceMethod *getMethod(const std::string &methodName) const;

  // 检查是否实现了另一个接口
  bool implements(const InterfaceType &other) const;

protected:
  bool isCompatibleWithImpl(const Type &other) const override;
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::string name;
  std::vector<std::string> typeParameters;                    // 泛型参数
  std::vector<std::shared_ptr<InterfaceType>> baseInterfaces; // 父接口列表
  std::unordered_map<std::string, InterfaceMethod> methods;   // 方法列表
};

} // namespace types
} // namespace c_hat