#pragma once

#include "Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

class ConditionalExpr : public Expression {
public:
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Expression> thenExpr;
  std::unique_ptr<Expression> elseExpr;

  ConditionalExpr(std::unique_ptr<Expression> condition,
                  std::unique_ptr<Expression> thenExpr,
                  std::unique_ptr<Expression> elseExpr)
      : condition(std::move(condition)), thenExpr(std::move(thenExpr)),
        elseExpr(std::move(elseExpr)) {}

  NodeType getType() const override { return NodeType::ConditionalExpr; }

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<ConditionalExpr>(condition->clone(),
                                             thenExpr->clone(),
                                             elseExpr->clone());
  }

  std::string toString() const override {
    return "(" + condition->toString() + " ? " + thenExpr->toString() +
           " : " + elseExpr->toString() + ")";
  }
};

} // namespace ast
} // namespace c_hat
