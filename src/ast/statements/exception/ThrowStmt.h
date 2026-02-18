#pragma once

#include "../../expressions/Expression.h"
#include "../Statement.h"
#include <memory>


namespace c_hat {
namespace ast {

// throw语句
class ThrowStmt : public Statement {
public:
  ThrowStmt(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::ThrowStmt; }
  std::string toString() const override;

  std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
