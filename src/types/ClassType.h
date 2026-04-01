#pragma once

#include "InterfaceType.h"
#include "Type.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace c_hat {
namespace types {

// 类方法信息
struct ClassMethod {
  std::string name;
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<Type>> paramTypes;
  bool isVirtual = false;
  bool isOverride = false;
  bool isStatic = false;                          // 是否为静态方法
  AccessModifier access = AccessModifier::Public; // 默认公共访问

  ClassMethod() = default;
  ClassMethod(const std::string &name, std::shared_ptr<Type> returnType,
              std::vector<std::shared_ptr<Type>> paramTypes,
              bool isVirtual = false, bool isOverride = false,
              AccessModifier access = AccessModifier::Public,
              bool isStatic = false)
      : name(name), returnType(returnType), paramTypes(paramTypes),
        isVirtual(isVirtual), isOverride(isOverride), isStatic(isStatic),
        access(access) {}

  // 添加拷贝构造函数
  ClassMethod(const ClassMethod &) = default;
  ClassMethod &operator=(const ClassMethod &) = default;
};

// 类字段信息
struct ClassField {
  std::string name;
  std::shared_ptr<Type> type;
  AccessModifier access = AccessModifier::Public; // 默认公共访问
  bool isStatic = false;                          // 是否为静态字段

  ClassField() = default;
  ClassField(const std::string &name, std::shared_ptr<Type> type,
             AccessModifier access = AccessModifier::Public,
             bool isStatic = false)
      : name(name), type(type), access(access), isStatic(isStatic) {}

  // 添加拷贝构造函数
  ClassField(const ClassField &) = default;
  ClassField &operator=(const ClassField &) = default;
};

// 类属性信息（带有 getter 和 setter）
struct ClassProperty {
  std::string name;
  std::shared_ptr<Type> type;
  AccessModifier access = AccessModifier::Public; // 默认公共访问
  bool hasGetter = false;
  bool hasSetter = false;

  ClassProperty() = default;
  ClassProperty(const std::string &name, std::shared_ptr<Type> type,
                AccessModifier access = AccessModifier::Public)
      : name(name), type(type), access(access) {}

  // 添加拷贝构造函数
  ClassProperty(const ClassProperty &) = default;
  ClassProperty &operator=(const ClassProperty &) = default;
};

// 类类型
class ClassType : public Type {
public:
  // 构造函数
  explicit ClassType(std::string name);

  // 带泛型参数的构造函数
  ClassType(std::string name, std::vector<std::string> typeParameters);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为类类型
  bool isClass() const override { return true; }

  // 获取类名
  const std::string &getName() const { return name; }

  // 获取泛型参数
  const std::vector<std::string> &getTypeParameters() const {
    return typeParameters;
  }
  void setTypeParameters(const std::vector<std::string> &params) {
    typeParameters = params;
  }
  bool isGeneric() const { return !typeParameters.empty(); }

  // 获取实例化后的类型参数
  const std::vector<std::shared_ptr<Type>> &getTypeArguments() const {
    return typeArguments;
  }
  void setTypeArguments(const std::vector<std::shared_ptr<Type>> &args) {
    typeArguments = args;
  }

  // 实例化泛型类型
  std::shared_ptr<ClassType>
  instantiate(const std::vector<std::shared_ptr<Type>> &typeArguments) const;

  // 添加基类
  void addBaseClass(std::shared_ptr<ClassType> baseClass);

  // 获取基类列表
  const std::vector<std::shared_ptr<ClassType>> &getBaseClasses() const {
    return baseClasses;
  }

  // 添加接口
  void addInterface(std::shared_ptr<InterfaceType> interface);

  // 获取接口列表
  const std::vector<std::shared_ptr<InterfaceType>> &getInterfaces() const {
    return interfaces;
  }

  // 添加方法
  void addMethod(const ClassMethod &method);
  const std::unordered_map<std::string, ClassMethod> &getMethods() const {
    return methods;
  }

  // 添加字段
  void addField(const ClassField &field);
  const std::unordered_map<std::string, ClassField> &getFields() const {
    return fields;
  }

  // 添加属性
  void addProperty(const ClassProperty &property);
  const std::unordered_map<std::string, ClassProperty> &getProperties() const {
    return properties;
  }

  // 查找方法（包括继承的方法）
  bool hasMethod(const std::string &methodName) const;
  const ClassMethod *getMethod(const std::string &methodName) const;

  // 查找字段（包括继承的字段）
  bool hasField(const std::string &fieldName) const;
  const ClassField *getField(const std::string &fieldName) const;
  std::shared_ptr<Type> getFieldType(const std::string &fieldName) const;

  // 查找属性（包括继承的属性）
  bool hasProperty(const std::string &propertyName) const;
  const ClassProperty *getProperty(const std::string &propertyName) const;
  std::shared_ptr<Type> getPropertyType(const std::string &propertyName) const;

  // 检查是否实现了某个接口
  bool implements(const InterfaceType &interface) const;

  // 获取未实现的接口方法
  std::vector<std::string>
  getUnimplementedMethods(const InterfaceType &interface) const;

  // 检测接口方法冲突
  std::vector<std::string> detectInterfaceMethodConflicts() const;

  // 检查方法签名是否相同
  bool isMethodSignatureSame(const ClassMethod &method1,
                             const InterfaceMethod &method2) const;
  bool isMethodSignatureSame(const InterfaceMethod &method1,
                             const InterfaceMethod &method2) const;

  // 检查是否有构造函数
  bool hasConstructor() const;

  // 检查是否有析构函数
  bool hasDestructor() const;

  // 检查是否为抽象类
  bool isAbstract() const { return isAbstract_; }
  void setAbstract(bool isAbstract) { isAbstract_ = isAbstract; }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::string name;
  std::vector<std::string> typeParameters;          // 泛型参数名
  std::vector<std::shared_ptr<Type>> typeArguments; // 实例化后的类型参数
  std::vector<std::shared_ptr<ClassType>> baseClasses;
  std::vector<std::shared_ptr<InterfaceType>> interfaces;
  std::unordered_map<std::string, ClassMethod> methods;      // 方法列表
  std::unordered_map<std::string, ClassField> fields;        // 字段列表
  std::unordered_map<std::string, ClassProperty> properties; // 属性列表
  bool isAbstract_ = false;                                  // 是否为抽象类
};

} // namespace types
} // namespace c_hat