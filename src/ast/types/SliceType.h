#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace ast {

// 切片类型
class SliceType : public Type {
public:
  SliceType(std::unique_ptr<Type> baseType) : baseType(std::move(baseType)) {}

  NodeType getType() const override { return NodeType::SliceType; }
  std::string toString() const override;

  std::unique_ptr<Type> baseType;
};

} // namespace ast
} // namespace c_hat
