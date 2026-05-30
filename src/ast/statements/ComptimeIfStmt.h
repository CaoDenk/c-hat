#pragma once

#include "Statement.h"
#include "../expressions/Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// 编译期 if - 只编译条件为真的分支
class ComptimeIfStmt : public Statement {
public:
  ComptimeIfStmt(std::unique_ptr<Expression> condition,
                 std::unique_ptr<Statement> thenBranch,
                 std::unique_ptr<Statement> elseBranch = nullptr)
      : condition(std::move(condition)), thenBranch(std::move(thenBranch)),
        elseBranch(std::move(elseBranch)) {}

  NodeType getType() const override { return NodeType::ComptimeIfStmt; }
  std::string toString() const override {
    std::string result = "comptime if (";
    if (condition) result += condition->toString();
    result += ") ";
    if (thenBranch) result += thenBranch->toString();
    if (elseBranch) {
      result += " else " + elseBranch->toString();
    }
    return result;
  }

  std::unique_ptr<Expression> condition;
  std::unique_ptr<Statement> thenBranch;
  std::unique_ptr<Statement> elseBranch;
};

} // namespace ast
} // namespace c_hat
