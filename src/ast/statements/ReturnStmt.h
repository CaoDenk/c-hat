#pragma once

#include "../expressions/Expression.h"
#include "Statement.h"
#include <memory>

namespace c_hat {
namespace ast {

// 返回语句
class ReturnStmt : public Statement {
public:
  ReturnStmt(std::unique_ptr<Expression> expr = nullptr)
      : expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::ReturnStmt; }
  std::string toString() const override;

  std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
