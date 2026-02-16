#pragma once

#include "Expression.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// 扩展表达式
class ExpansionExpr : public Expression {
public:
  ExpansionExpr(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::ExpansionExpr; }
  std::string toString() const override;

  std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
