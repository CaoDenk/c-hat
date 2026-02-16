#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace ast {

// 只读类型（如 byte!）
class ReadonlyType : public Type {
public:
  ReadonlyType(std::unique_ptr<Type> baseType);

  NodeType getType() const override { return NodeType::ReadonlyType; }
  std::string toString() const override;

  std::unique_ptr<Type> baseType;
};

} // namespace ast
} // namespace c_hat
