#pragma once

#include "Expression.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace ast {

// new 表达式
class NewExpr : public Expression {
public:
  NewExpr(std::unique_ptr<Node> type,
          std::vector<std::unique_ptr<Expression>> args)
      : type(std::move(type)), args(std::move(args)) {}

  NodeType getType() const override { return NodeType::NewExpr; }
  std::string toString() const override;

  std::unique_ptr<Node> type;
  std::vector<std::unique_ptr<Expression>> args;
};

} // namespace ast
} // namespace c_hat
