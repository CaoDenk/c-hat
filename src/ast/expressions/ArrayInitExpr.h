#pragma once

#include "Expression.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

// 数组初始化表达式
class ArrayInitExpr : public Expression {
public:
  ArrayInitExpr(std::vector<std::unique_ptr<Expression>> elements)
      : elements(std::move(elements)) {}

  NodeType getType() const override { return NodeType::ArrayInitExpr; }
  std::string toString() const override;

  std::vector<std::unique_ptr<Expression>> elements;
};

} // namespace ast
} // namespace c_hat
