#pragma once

#include "Expression.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// 二元表达式
class BinaryExpr : public Expression {
public:
  enum class Op {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,
    Or,
    Xor,
    Shl,
    Shr,
    Lt,
    Le,
    Gt,
    Ge,
    Eq,
    Ne,
    LogicAnd,
    LogicOr,
    Assign,
    AddAssign,
    SubAssign,
    MulAssign,
    DivAssign,
    ModAssign,
    AndAssign,
    OrAssign,
    XorAssign,
    ShlAssign,
    ShrAssign,
    Range
  };

  BinaryExpr(std::unique_ptr<Expression> left, Op op,
             std::unique_ptr<Expression> right)
      : left(std::move(left)), op(op), right(std::move(right)) {}

  NodeType getType() const override { return NodeType::BinaryExpr; }
  std::string toString() const override;

  std::unique_ptr<Expression> left;
  Op op;
  std::unique_ptr<Expression> right;
};

} // namespace ast
} // namespace c_hat
