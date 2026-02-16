#pragma once

#include "Type.h"

namespace c_hat {
namespace types {

// LiteralView 类型
class LiteralViewType : public Type {
public:
  LiteralViewType() = default;

  // 将类型转换为字符串表示
  std::string toString() const override;

  // 检查是否为 LiteralView 类型
  bool isLiteralView() const override { return true; }

protected:
  // 具体类型的兼容性检查实现
  bool isCompatibleWithImpl(const Type &other) const override;

  // 具体类型的子类型关系检查实现
  bool isSubtypeOfImpl(const Type &other) const override;
};

} // namespace types
} // namespace c_hat
