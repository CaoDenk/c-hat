#pragma once

#include "../expressions/Expression.h"
#include "Type.h"
#include <memory>

namespace c_hat {
namespace ast {

// 数组类型
class ArrayType : public Type {
public:
  ArrayType(std::unique_ptr<Type> baseType, std::unique_ptr<Expression> size)
      : baseType(std::move(baseType)), size(std::move(size)) {}

  NodeType getType() const override { return NodeType::ArrayType; }
  std::string toString() const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<ArrayType>(baseType->clone(), size->clone());
  }

  std::unique_ptr<Type> baseType;
  std::unique_ptr<Expression> size;
};

} // namespace ast
} // namespace c_hat
