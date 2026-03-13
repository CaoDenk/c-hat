#include "InterfaceType.h"
#include "ClassType.h"

namespace c_hat {
namespace types {

InterfaceType::InterfaceType(std::string name) : name(std::move(name)) {}

InterfaceType::InterfaceType(std::string name,
                             std::vector<std::string> typeParameters)
    : name(std::move(name)), typeParameters(std::move(typeParameters)) {}

std::string InterfaceType::toString() const {
  if (typeParameters.empty()) {
    return name;
  }
  std::string result = name + "<";
  for (size_t i = 0; i < typeParameters.size(); ++i) {
    if (i > 0)
      result += ", ";
    result += typeParameters[i];
  }
  result += ">";
  return result;
}

void InterfaceType::addBaseInterface(
    std::shared_ptr<InterfaceType> baseInterface) {
  baseInterfaces.push_back(baseInterface);
}

void InterfaceType::addMethod(const InterfaceMethod &method) {
  methods[method.name] = method;
}

std::shared_ptr<InterfaceType> InterfaceType::instantiate(
    const std::vector<std::shared_ptr<Type>> &typeArguments) const {
  if (typeArguments.size() != typeParameters.size()) {
    return nullptr; // 类型参数数量不匹配
  }

  // 创建新的接口类型，使用类型实参替换类型参数
  std::vector<std::string> instantiatedTypeParams;
  for (const auto &typeArg : typeArguments) {
    instantiatedTypeParams.push_back(typeArg->toString());
  }
  auto instantiatedInterface =
      std::make_shared<InterfaceType>(name, instantiatedTypeParams);

  // 复制父接口
  for (const auto &baseInterface : baseInterfaces) {
    instantiatedInterface->addBaseInterface(baseInterface);
  }

  // 复制方法，替换类型参数
  for (const auto &[methodName, method] : methods) {
    InterfaceMethod instantiatedMethod = method;
    // 这里可以添加类型参数替换逻辑
    instantiatedInterface->addMethod(instantiatedMethod);
  }

  return instantiatedInterface;
}

bool InterfaceType::hasMethod(const std::string &methodName) const {
  if (methods.find(methodName) != methods.end()) {
    return true;
  }
  // 在父接口中查找
  for (const auto &baseInterface : baseInterfaces) {
    if (baseInterface->hasMethod(methodName)) {
      return true;
    }
  }
  return false;
}

const InterfaceMethod *
InterfaceType::getMethod(const std::string &methodName) const {
  auto it = methods.find(methodName);
  if (it != methods.end()) {
    return &(it->second);
  }
  // 在父接口中查找
  for (const auto &baseInterface : baseInterfaces) {
    const InterfaceMethod *method = baseInterface->getMethod(methodName);
    if (method != nullptr) {
      return method;
    }
  }
  return nullptr;
}

bool InterfaceType::implements(const InterfaceType &other) const {
  if (name == other.name) {
    return true;
  }
  // 检查是否通过父接口实现
  for (const auto &baseInterface : baseInterfaces) {
    if (baseInterface->implements(other)) {
      return true;
    }
  }
  return false;
}

bool InterfaceType::isCompatibleWithImpl(const Type &other) const {
  if (this == &other) {
    return true;
  }
  if (other.isInterface()) {
    const auto &otherInterface = static_cast<const InterfaceType &>(other);
    // 检查是否实现了另一个接口
    return implements(otherInterface);
  }
  if (other.isClass()) {
    // 检查类是否实现了此接口
    const auto &otherClass = static_cast<const ClassType &>(other);
    // 这里需要检查类是否实现了此接口，暂时返回 false
    // 后续需要在 ClassType 中添加接口信息
    return false;
  }
  return false;
}

bool InterfaceType::isSubtypeOfImpl(const Type &other) const {
  if (isCompatibleWithImpl(other)) {
    return true;
  }
  if (other.isInterface()) {
    const auto &otherInterface = static_cast<const InterfaceType &>(other);
    // 检查是否实现了另一个接口
    return implements(otherInterface);
  }
  return false;
}

} // namespace types
} // namespace c_hat