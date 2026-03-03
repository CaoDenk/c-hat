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
  std::unique_ptr<Expression> clone() const override {
    std::vector<std::unique_ptr<Expression>> clonedArgs;
    for (const auto &arg : args) {
      clonedArgs.push_back(arg->clone());
    }
    // 注意：这里我们暂时无法克隆 Node 类型的 type，因为它没有 clone 方法
    // 这是一个临时解决方案，实际应该为 Node 类也添加 clone 方法
    return std::make_unique<NewExpr>(nullptr, std::move(clonedArgs));
  }

  std::unique_ptr<Node> type;
  std::vector<std::unique_ptr<Expression>> args;
};

} // namespace ast
} // namespace c_hat
