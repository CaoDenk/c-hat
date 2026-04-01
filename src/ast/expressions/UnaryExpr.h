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
    At,
    ForceUnwrap,
    NullablePropagation,
    PreIncrement,
    PreDecrement,
    PostIncrement,
    PostDecrement
  };

  UnaryExpr(Op op, std::unique_ptr<Expression> expr)
      : op(op), expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::UnaryExpr; }
  std::string toString() const override;
  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<UnaryExpr>(op, expr->clone());
  }

  Op op;
  std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
