#pragma once

#include "Expression.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace ast {

// 函数调用
class CallExpr : public Expression {
public:
  CallExpr(std::unique_ptr<Expression> callee,
           std::vector<std::unique_ptr<Expression>> args)
      : callee(std::move(callee)), args(std::move(args)) {}

  NodeType getType() const override { return NodeType::CallExpr; }
  std::string toString() const override;

  std::unique_ptr<Expression> callee;
  std::vector<std::unique_ptr<Expression>> args;
};

} // namespace ast
} // namespace c_hat
