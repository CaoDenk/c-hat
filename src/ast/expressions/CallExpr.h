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
  std::unique_ptr<Expression> clone() const override {
    std::vector<std::unique_ptr<Expression>> clonedArgs;
    for (const auto &arg : args) {
      clonedArgs.push_back(arg->clone());
    }
    return std::make_unique<CallExpr>(callee->clone(), std::move(clonedArgs));
  }

  std::unique_ptr<Expression> callee;
  std::vector<std::unique_ptr<Expression>> args;
};

} // namespace ast
} // namespace c_hat
