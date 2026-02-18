#pragma once

#include "Expression.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace ast {

class TupleExpr : public Expression {
public:
  std::vector<std::unique_ptr<Expression>> elements;

  TupleExpr(std::vector<std::unique_ptr<Expression>> elements)
      : elements(std::move(elements)) {}

  NodeType getType() const override { return NodeType::TupleExpr; }

  std::string toString() const override {
    std::string result = "(";
    for (size_t i = 0; i < elements.size(); ++i) {
      if (i > 0) {
        result += ", ";
      }
      result += elements[i]->toString();
    }
    result += ")";
    return result;
  }
};

} // namespace ast
} // namespace c_hat
