#pragma once

#include "Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// 一元表达式
class UnaryExpr : public Expression {
public:
  enum class Op {
    Plus,
    Minus,
    Not,
    BitNot,
    AddressOf,
    Dereference,
    Ref,
    Await,
    Move,
    Immutable,
    At
  };

  UnaryExpr(Op op, std::unique_ptr<Expression> expr)
      : op(op), expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::UnaryExpr; }
  std::string toString() const override;

  Op op;
  std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
