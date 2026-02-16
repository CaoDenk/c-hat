#pragma once

#include "../Statement.h"
#include "../../expressions/Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// defer语句
class DeferStmt : public Statement {
public:
  DeferStmt(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::DeferStmt; }
  std::string toString() const override;

  std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
