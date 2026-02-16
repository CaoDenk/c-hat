#pragma once

#include "../expressions/Expression.h"
#include "Statement.h"
#include <memory>

namespace c_hat {
namespace ast {

// while语句
class WhileStmt : public Statement {
public:
  WhileStmt(std::unique_ptr<Expression> condition,
            std::unique_ptr<Statement> body)
      : condition(std::move(condition)), body(std::move(body)) {}

  NodeType getType() const override { return NodeType::WhileStmt; }
  std::string toString() const override;

  std::unique_ptr<Expression> condition;
  std::unique_ptr<Statement> body;
};

} // namespace ast
} // namespace c_hat
