#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace ast {

// 指针类型
class PointerType : public Type {
public:
  PointerType(std::unique_ptr<Type> baseType, bool isNullable = false)
      : baseType(std::move(baseType)), isNullable(isNullable) {}

  NodeType getType() const override { return NodeType::PointerType; }
  std::string toString() const override;

  std::unique_ptr<Type> baseType;
  bool isNullable;
};

} // namespace ast
} // namespace c_hat
