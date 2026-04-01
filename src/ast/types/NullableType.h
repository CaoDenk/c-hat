#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace ast {

class NullableType : public Type {
public:
  explicit NullableType(std::unique_ptr<Type> baseType)
      : baseType(std::move(baseType)) {}

  NodeType getType() const override { return NodeType::NullableType; }
  std::string toString() const override {
    return baseType ? baseType->toString() + "?" : "?";
  }
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<NullableType>(baseType ? baseType->clone() : nullptr);
  }

  std::unique_ptr<Type> baseType;
};

} // namespace ast
} // namespace c_hat
