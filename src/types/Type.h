#pragma once

#include <memory>
#include <string>

namespace c_hat {
namespace types {

// 类型基类
class Type {
public:
  virtual ~Type() = default;

  // 将类型转换为字符串表示
  virtual std::string toString() const = 0;

  // 检查类型兼容性（先剥除 Readonly 包装）
  bool isCompatibleWith(const Type &other) const;

  // 检查子类型关系（先剥除 Readonly 包装）
  bool isSubtypeOf(const Type &other) const;

  // 检查是否为基本类型
  virtual bool isPrimitive() const { return false; }

  // 检查是否为数组类型
  virtual bool isArray() const { return false; }

  // 检查是否为切片类型
  virtual bool isSlice() const { return false; }

  // 检查是否为指针类型
  virtual bool isPointer() const { return false; }

  // 检查是否为函数类型
  virtual bool isFunction() const { return false; }

  // 检查是否为类类型
  virtual bool isClass() const { return false; }

  // 检查是否为泛型类型
  virtual bool isGeneric() const { return false; }

  // 检查是否为元组类型
  virtual bool isTuple() const { return false; }

  // 检查是否为void类型
  virtual bool isVoid() const { return false; }

  // 检查是否为 LiteralView 类型
  virtual bool isLiteralView() const { return false; }

  // 检查是否为只读类型
  virtual bool isReadonly() const { return false; }

  // 如果是只读类型，获取基础类型（非只读的）
  virtual std::shared_ptr<Type> getBaseType() const { return nullptr; }

protected:
  // 具体类型的兼容性检查实现（不包含 Readonly 包装处理）
  virtual bool isCompatibleWithImpl(const Type &other) const = 0;

  // 具体类型的子类型关系检查实现（不包含 Readonly 包装处理）
  virtual bool isSubtypeOfImpl(const Type &other) const = 0;
};

// 辅助函数：剥除所有 Readonly 包装，返回基础类型
std::shared_ptr<const Type> unwrapReadonly(const Type *type);
std::shared_ptr<Type> unwrapReadonly(std::shared_ptr<Type> type);

} // namespace types
} // namespace c_hat
