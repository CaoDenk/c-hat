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
  // 语义分析阶段填充的重载解析结果（mangled 函数名）
  // 由 SemanticAnalyzer 在分析 CallExpr 时计算并设置
  std::string resolvedMangledName;
};

} // namespace ast
} // namespace c_hat
