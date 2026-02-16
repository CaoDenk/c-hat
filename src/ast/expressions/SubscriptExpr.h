#pragma once

#include "Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// 下标访问
class SubscriptExpr : public Expression {
public:
  SubscriptExpr(std::unique_ptr<Expression> object,
                std::unique_ptr<Expression> index)
      : object(std::move(object)), index(std::move(index)) {}

  NodeType getType() const override { return NodeType::SubscriptExpr; }
  std::string toString() const override;

  std::unique_ptr<Expression> object;
  std::unique_ptr<Expression> index;
};

} // namespace ast
} // namespace c_hat
