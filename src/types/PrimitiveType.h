#pragma once

#include "Type.h"
#include <string>

namespace c_hat {
namespace types {

// 基本类型
class PrimitiveType : public Type {
public:
  // 基本类型枚举
  enum class Kind {
    Void,
    Bool,
    Byte,
    SByte,
    Short,
    UShort,
    Int,
    UInt,
    Long,
    ULong,
    Float,
    Double,
    Fp16,
    Bf16,
    Char
  };

  // 构造函数
  explicit PrimitiveType(Kind kind);

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为基本类型
  bool isPrimitive() const override { return true; }

  // 检查是否为void类型
  bool isVoid() const override { return kind == Kind::Void; }

  // 获取类型枚举值
  Kind getKind() const { return kind; }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  Kind kind;
};

} // namespace types
} // namespace c_hat
