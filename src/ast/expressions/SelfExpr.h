#pragma once

#include "Expression.h"

namespace c_hat {
namespace ast {

// self 表达式
class SelfExpr : public Expression {
public:
  NodeType getType() const override { return NodeType::SelfExpr; }
  std::string toString() const override { return "self"; }
};

} // namespace ast
} // namespace c_hat
