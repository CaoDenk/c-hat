#pragma once

#include "../Node.h"
#include "RequiresExpression.h"
#include <memory>

namespace c_hat {
namespace ast {

// requires子句
class RequiresClause : public Node {
public:
  RequiresClause(std::unique_ptr<RequiresExpression> expr)
      : expr(std::move(expr)) {}

  NodeType getType() const override { return NodeType::RequiresClause; }
  std::string toString() const override;

  std::unique_ptr<RequiresExpression> expr; // requires表达式
};

} // namespace ast
} // namespace c_hat
