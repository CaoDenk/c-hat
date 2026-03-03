#pragma once

#include "Expression.h"

namespace c_hat {
namespace ast {

// this 表达式
class ThisExpr : public Expression {
public:
  NodeType getType() const override { return NodeType::ThisExpr; }
  std::string toString() const override { return "this"; }
  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<ThisExpr>();
  }
};

} // namespace ast
} // namespace c_hat
