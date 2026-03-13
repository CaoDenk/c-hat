#include "ClassType.h"

namespace c_hat {
namespace types {

ClassType::ClassType(std::string name) : name(name) {}

ClassType::ClassType(std::string name, std::vector<std::string> typeParameters)
    : name(name), typeParameters(std::move(typeParameters)) {}

std::string ClassType::toString() const {
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

void ClassType::addBaseClass(std::shared_ptr<ClassType> baseClass) {
  baseClasses.push_back(baseClass);
}

void ClassType::addInterface(std::shared_ptr<InterfaceType> interface) {
  interfaces.push_back(interface);
}

void ClassType::addMethod(const ClassMethod &method) {
  methods[method.name] = method;
}

void ClassType::addField(const ClassField &field) {
  fields[field.name] = field;
}

void ClassType::addProperty(const ClassProperty &property) {
  properties[property.name] = property;
}

std::shared_ptr<ClassType> ClassType::instantiate(
    const std::vector<std::shared_ptr<Type>> &typeArguments) const {
  if (typeArguments.size() != typeParameters.size()) {
    return nullptr; // 类型参数数量不匹配
  }

  // 创建新的类类型，使用类型实参替换类型参数
  std::vector<std::string> instantiatedTypeParams;
  for (const auto &typeArg : typeArguments) {
    instantiatedTypeParams.push_back(typeArg->toString());
  }
  auto instantiatedClass =
      std::make_shared<ClassType>(name, instantiatedTypeParams);

  // 复制基类和接口
  for (const auto &baseClass : baseClasses) {
    instantiatedClass->addBaseClass(baseClass);
  }
  for (const auto &interface : interfaces) {
    instantiatedClass->addInterface(interface);
  }

  // 复制方法，替换类型参数
  for (const auto &[methodName, method] : methods) {
    ClassMethod instantiatedMethod = method;
    // 这里可以添加类型参数替换逻辑
    instantiatedClass->addMethod(instantiatedMethod);
  }

  // 复制字段，替换类型参数
  for (const auto &[fieldName, field] : fields) {
    ClassField instantiatedField = field;
    // 这里可以添加类型参数替换逻辑
    instantiatedClass->addField(instantiatedField);
  }

  // 复制属性，替换类型参数
  for (const auto &[propertyName, property] : properties) {
    ClassProperty instantiatedProperty = property;
    // 这里可以添加类型参数替换逻辑
    instantiatedClass->addProperty(instantiatedProperty);
  }

  return instantiatedClass;
}

bool ClassType::hasMethod(const std::string &methodName) const {
  if (methods.find(methodName) != methods.end()) {
    return true;
  }
  // 在基类中查找
  for (const auto &baseClass : baseClasses) {
    if (baseClass->hasMethod(methodName)) {
      return true;
    }
  }
  return false;
}

const ClassMethod *ClassType::getMethod(const std::string &methodName) const {
  auto it = methods.find(methodName);
  if (it != methods.end()) {
    return &(it->second);
  }
  // 在基类中查找
  for (const auto &baseClass : baseClasses) {
    const ClassMethod *method = baseClass->getMethod(methodName);
    if (method != nullptr) {
      return method;
    }
  }
  return nullptr;
}

bool ClassType::hasField(const std::string &fieldName) const {
  if (fields.find(fieldName) != fields.end()) {
    return true;
  }
  // 在基类中查找
  for (const auto &baseClass : baseClasses) {
    if (baseClass->hasField(fieldName)) {
      return true;
    }
  }
  return false;
}

const ClassField *ClassType::getField(const std::string &fieldName) const {
  auto it = fields.find(fieldName);
  if (it != fields.end()) {
    return &(it->second);
  }
  // 在基类中查找
  for (const auto &baseClass : baseClasses) {
    const ClassField *field = baseClass->getField(fieldName);
    if (field != nullptr) {
      return field;
    }
  }
  return nullptr;
}

std::shared_ptr<Type>
ClassType::getFieldType(const std::string &fieldName) const {
  const ClassField *field = getField(fieldName);
  if (field != nullptr) {
    return field->type;
  }
  return nullptr;
}

bool ClassType::hasProperty(const std::string &propertyName) const {
  if (properties.find(propertyName) != properties.end()) {
    return true;
  }
  // 在基类中查找
  for (const auto &baseClass : baseClasses) {
    if (baseClass->hasProperty(propertyName)) {
      return true;
    }
  }
  return false;
}

const ClassProperty *
ClassType::getProperty(const std::string &propertyName) const {
  auto it = properties.find(propertyName);
  if (it != properties.end()) {
    return &(it->second);
  }
  // 在基类中查找
  for (const auto &baseClass : baseClasses) {
    const ClassProperty *property = baseClass->getProperty(propertyName);
    if (property != nullptr) {
      return property;
    }
  }
  return nullptr;
}

std::shared_ptr<Type>
ClassType::getPropertyType(const std::string &propertyName) const {
  const ClassProperty *property = getProperty(propertyName);
  if (property != nullptr) {
    return property->type;
  }
  return nullptr;
}

bool ClassType::implements(const InterfaceType &interface) const {
  // 检查是否直接实现了接口
  for (const auto &iface : interfaces) {
    if (iface->implements(interface)) {
      return true;
    }
  }
  // 检查基类是否实现了接口
  for (const auto &baseClass : baseClasses) {
    if (baseClass->implements(interface)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string>
ClassType::getUnimplementedMethods(const InterfaceType &interface) const {
  std::vector<std::string> unimplemented;

  // 获取接口的所有方法（包括继承的方法）
  for (const auto &method : interface.getMethods()) {
    const std::string &methodName = method.first;
    bool hasImplementation = hasMethod(methodName);

    // 如果方法有默认实现，则不需要强制实现
    if (!hasImplementation && !method.second.hasDefaultImplementation) {
      unimplemented.push_back(methodName);
    }
  }

  // 检查父接口的方法
  for (const auto &baseInterface : interface.getBaseInterfaces()) {
    auto baseUnimplemented = getUnimplementedMethods(*baseInterface);
    unimplemented.insert(unimplemented.end(), baseUnimplemented.begin(),
                         baseUnimplemented.end());
  }

  return unimplemented;
}

bool ClassType::isCompatibleWithImpl(const Type &other) const {
  if (this == &other) {
    return true;
  }
  if (other.isClass()) {
    const auto &otherClass = static_cast<const ClassType &>(other);
    return name == otherClass.name;
  }
  if (other.isInterface()) {
    // 检查类是否实现了此接口
    const auto &otherInterface = static_cast<const InterfaceType &>(other);
    return implements(otherInterface);
  }
  return false;
}

bool ClassType::isSubtypeOfImpl(const Type &other) const {
  if (isCompatibleWithImpl(other)) {
    return true;
  }
  if (other.isClass()) {
    const auto &otherClass = static_cast<const ClassType &>(other);
    for (const auto &baseClass : baseClasses) {
      if (baseClass->isSubtypeOf(otherClass)) {
        return true;
      }
    }
  }
  if (other.isInterface()) {
    // 检查类是否实现了此接口
    const auto &otherInterface = static_cast<const InterfaceType &>(other);
    return implements(otherInterface);
  }
  return false;
}

// 收集接口及其父接口的所有方法
void collectInterfaceMethods(
    const InterfaceType &interface,
    std::unordered_map<std::string, std::vector<const InterfaceMethod *>>
        &methodMap) {
  // 添加当前接口的方法
  for (const auto &[methodName, method] : interface.getMethods()) {
    methodMap[methodName].push_back(&method);
  }

  // 递归收集父接口的方法
  for (const auto &baseInterface : interface.getBaseInterfaces()) {
    collectInterfaceMethods(*baseInterface, methodMap);
  }
}

std::vector<std::string> ClassType::detectInterfaceMethodConflicts() const {
  std::vector<std::string> conflicts;

  // 收集所有接口的方法
  std::unordered_map<std::string, std::vector<const InterfaceMethod *>>
      methodMap;
  for (const auto &interface : interfaces) {
    collectInterfaceMethods(*interface, methodMap);
  }

  // 检查每个方法名是否有冲突
  for (const auto &[methodName, methods] : methodMap) {
    if (methods.size() > 1) {
      // 检查方法签名是否一致
      const InterfaceMethod *firstMethod = methods[0];
      bool hasConflict = false;

      for (size_t i = 1; i < methods.size(); ++i) {
        if (!isMethodSignatureSame(*firstMethod, *methods[i])) {
          hasConflict = true;
          break;
        }
      }

      if (hasConflict) {
        conflicts.push_back(methodName);
      }
    }
  }

  return conflicts;
}

bool ClassType::isMethodSignatureSame(const ClassMethod &method1,
                                      const InterfaceMethod &method2) const {
  // 检查返回类型
  if (method1.returnType->toString() != method2.returnType->toString()) {
    return false;
  }

  // 检查参数数量
  if (method1.paramTypes.size() != method2.paramTypes.size()) {
    return false;
  }

  // 检查参数类型
  for (size_t i = 0; i < method1.paramTypes.size(); ++i) {
    if (method1.paramTypes[i]->toString() !=
        method2.paramTypes[i]->toString()) {
      return false;
    }
  }

  return true;
}

bool ClassType::isMethodSignatureSame(const InterfaceMethod &method1,
                                      const InterfaceMethod &method2) const {
  // 检查返回类型
  if (method1.returnType->toString() != method2.returnType->toString()) {
    return false;
  }

  // 检查参数数量
  if (method1.paramTypes.size() != method2.paramTypes.size()) {
    return false;
  }

  // 检查参数类型
  for (size_t i = 0; i < method1.paramTypes.size(); ++i) {
    if (method1.paramTypes[i]->toString() !=
        method2.paramTypes[i]->toString()) {
      return false;
    }
  }

  return true;
}

bool ClassType::hasConstructor() const {
  // 构造函数的名称与类名相同
  return hasMethod(name);
}

bool ClassType::hasDestructor() const {
  // 析构函数的名称是类名前面加上波浪号
  std::string destructorName = "~" + name;
  return hasMethod(destructorName);
}

} // namespace types
} // namespace c_hat