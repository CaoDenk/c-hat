#pragma once

#include "Expression.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace c_hat {
namespace ast {

// 结构体初始化表达式
class StructInitExpr : public Expression {
public:
  StructInitExpr(
      std::vector<std::pair<std::string, std::unique_ptr<Expression>>> fields)
      : fields(std::move(fields)) {}

  NodeType getType() const override { return NodeType::StructInitExpr; }
  std::string toString() const override;

  std::vector<std::pair<std::string, std::unique_ptr<Expression>>> fields;
};

} // namespace ast
} // namespace c_hat
