#pragma once

#include "Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

class TypeofExpr : public Expression {
public:
  TypeofExpr(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::TypeofExpr; }

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<TypeofExpr>(expr->clone());
  }

  std::string toString() const override {
    return "typeof(" + expr->toString() + ")";
  }

  std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
