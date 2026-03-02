#pragma once

#include "Expression.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// delete 表达式
class DeleteExpr : public Expression {
public:
  DeleteExpr(std::unique_ptr<Expression> expr, bool isArray = false)
      : expr(std::move(expr)), isArray(isArray) {}

  NodeType getType() const override { return NodeType::DeleteExpr; }
  std::string toString() const override;

  std::unique_ptr<Expression> expr;
  bool isArray;
  std::string typeName;  // 类型名（用于查找析构函数）
};

} // namespace ast
} // namespace c_hat
