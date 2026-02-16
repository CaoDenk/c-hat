#pragma once

#include "Expression.h"

namespace c_hat {
namespace ast {

// this 表达式
class ThisExpr : public Expression {
public:
  NodeType getType() const override { return NodeType::ThisExpr; }
  std::string toString() const override { return "this"; }
};

} // namespace ast
} // namespace c_hat
