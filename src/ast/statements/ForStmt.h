#pragma once

#include "../expressions/Expression.h"
#include "Statement.h"
#include <memory>

namespace c_hat {
namespace ast {

// 循环语句
class ForStmt : public Statement {
public:
  ForStmt(std::unique_ptr<Node> init, std::unique_ptr<Expression> condition,
          std::unique_ptr<Expression> update, std::unique_ptr<Statement> body)
      : init(std::move(init)), condition(std::move(condition)),
        update(std::move(update)), body(std::move(body)) {}

  NodeType getType() const override { return NodeType::ForStmt; }
  std::string toString() const override;

  std::unique_ptr<Node> init;
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Expression> update;
  std::unique_ptr<Statement> body;
};

} // namespace ast
} // namespace c_hat
