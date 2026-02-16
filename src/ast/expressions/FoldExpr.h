#pragma once

#include "Expression.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// 折叠表达式
class FoldExpr : public Expression {
public:
  enum class FoldType { Right, Left, Binary };

  FoldExpr(FoldType foldType, std::unique_ptr<Expression> expr,
           const std::string &op, std::unique_ptr<Expression> right = nullptr)
      : foldType(foldType), expr(std::move(expr)), op(op),
        right(std::move(right)) {}

  NodeType getType() const override { return NodeType::FoldExpr; }
  std::string toString() const override;

  FoldType foldType;
  std::unique_ptr<Expression> expr;
  std::string op;
  std::unique_ptr<Expression> right;
};

} // namespace ast
} // namespace c_hat
