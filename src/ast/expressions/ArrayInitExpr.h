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
  std::unique_ptr<Expression> clone() const override {
    std::vector<std::unique_ptr<Expression>> clonedElements;
    for (const auto &element : elements) {
      clonedElements.push_back(element->clone());
    }
    return std::make_unique<ArrayInitExpr>(std::move(clonedElements));
  }

  std::vector<std::unique_ptr<Expression>> elements;
};

} // namespace ast
} // namespace c_hat
