#pragma once

#include "Expression.h"

namespace c_hat {
namespace ast {

// super 表达式
class SuperExpr : public Expression {
public:
  NodeType getType() const override { return NodeType::SuperExpr; }
  std::string toString() const override { return "super"; }
};

} // namespace ast
} // namespace c_hat
