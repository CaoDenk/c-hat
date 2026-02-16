#pragma once

#include "../../expressions/Expression.h"
#include "../Statement.h"
#include "MatchArm.h"
#include "Pattern.h"
#include <memory>
#include <vector>


namespace c_hat {
namespace ast {

// 匹配语句
class MatchStmt : public Statement {
public:
  MatchStmt(std::unique_ptr<Expression> expr,
            std::vector<std::unique_ptr<MatchArm>> arms)
      : expr(std::move(expr)), arms(std::move(arms)) {}

  NodeType getType() const override { return NodeType::MatchStmt; }
  std::string toString() const override;

  std::unique_ptr<Expression> expr;
  std::vector<std::unique_ptr<MatchArm>> arms;
};

} // namespace ast
} // namespace c_hat
