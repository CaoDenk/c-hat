#pragma once

#include "../types/Type.h"
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

  StructInitExpr(
      std::unique_ptr<ast::Type> type,
      std::vector<std::pair<std::string, std::unique_ptr<Expression>>> fields)
      : type(std::move(type)), fields(std::move(fields)) {}

  NodeType getType() const override { return NodeType::StructInitExpr; }
  std::string toString() const override;
  std::unique_ptr<Expression> clone() const override {
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> clonedFields;
    for (const auto &field : fields) {
      clonedFields.push_back({field.first, field.second->clone()});
    }
    // 注意：这里我们暂时无法克隆 ast::Type 类型的 type，因为它没有 clone 方法
    // 这是一个临时解决方案，实际应该为 ast::Type 类也添加 clone 方法
    return std::make_unique<StructInitExpr>(nullptr, std::move(clonedFields));
  }

  std::unique_ptr<ast::Type> type;
  std::vector<std::pair<std::string, std::unique_ptr<Expression>>> fields;
};

} // namespace ast
} // namespace c_hat
