#pragma once

#include "../expressions/Expression.h"
#include "Statement.h"
#include <memory>

namespace c_hat {
namespace ast {

// 表达式语句
class ExprStmt : public Statement {
public:
  ExprStmt(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::ExprStmt; }
  std::string toString() const override;

  std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
