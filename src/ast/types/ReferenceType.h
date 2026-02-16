#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace ast {

class ReferenceType : public Type {
public:
  ReferenceType(std::unique_ptr<Type> baseType)
      : baseType(std::move(baseType)) {}

  NodeType getType() const override { return NodeType::ReferenceType; }
  std::string toString() const override;

  std::unique_ptr<Type> baseType;
};

} // namespace ast
} // namespace c_hat
