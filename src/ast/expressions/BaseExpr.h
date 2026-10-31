#pragma once

#include "Expression.h"

namespace c_hat {
namespace ast {

// base 表达式
class BaseExpr : public Expression {
public:
  NodeType getType() const override { return NodeType::BaseExpr; }
  std::string toString() const override { return "base"; }
  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<BaseExpr>();
  }
};

} // namespace ast
} // namespace c_hat
