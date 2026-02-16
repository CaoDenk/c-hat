#pragma once

#include "Type.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace ast {

class FunctionType : public Type {
public:
  FunctionType(std::vector<std::unique_ptr<Type>> parameterTypes,
               std::unique_ptr<Type> returnType)
      : parameterTypes(std::move(parameterTypes)),
        returnType(std::move(returnType)) {}

  NodeType getType() const override { return NodeType::FunctionType; }
  std::string toString() const override;

  std::vector<std::unique_ptr<Type>> parameterTypes;
  std::unique_ptr<Type> returnType;
};

} // namespace ast
} // namespace c_hat
