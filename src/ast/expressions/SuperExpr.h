#pragma once

#include "Expression.h"

namespace c_hat {
namespace ast {

// super 表达式
class SuperExpr : public Expression {
public:
  NodeType getType() const override { return NodeType::SuperExpr; }
  std::string toString() const override { return "super"; }
  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<SuperExpr>();
  }
};

} // namespace ast
} // namespace c_hat
