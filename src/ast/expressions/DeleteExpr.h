#pragma once

#include "Expression.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// delete 表达式
class DeleteExpr : public Expression {
public:
  DeleteExpr(std::unique_ptr<Expression> expr, bool isArray = false)
      : expr(std::move(expr)), isArray(isArray) {}

  NodeType getType() const override { return NodeType::DeleteExpr; }
  std::string toString() const override;

  std::unique_ptr<Expression> expr;
  bool isArray;
};

} // namespace ast
} // namespace c_hat
