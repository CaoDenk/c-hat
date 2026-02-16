#pragma once

#include "../expressions/Expression.h"
#include "Statement.h"
#include <memory>

namespace c_hat {
namespace ast {

// do-while语句
class DoWhileStmt : public Statement {
public:
  DoWhileStmt(std::unique_ptr<Statement> body,
              std::unique_ptr<Expression> condition)
      : body(std::move(body)), condition(std::move(condition)) {}

  NodeType getType() const override { return NodeType::DoWhileStmt; }
  std::string toString() const override;

  std::unique_ptr<Statement> body;
  std::unique_ptr<Expression> condition;
};

} // namespace ast
} // namespace c_hat
