#pragma once

#include "../types/Type.h"
#include "Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// 反射表达式: @Type 或 @typeof(expr)
// 用于在编译期获取类型的元数据
class ReflectionExpr : public Expression {
public:
  // 反射目标可以是类型或表达式
  enum class TargetKind {
    Type,  // @TypeName
    Typeof // @typeof(expr)
  };

  // 用于 @TypeName 的构造函数
  explicit ReflectionExpr(std::unique_ptr<ast::Type> targetType)
      : kind(TargetKind::Type), type(std::move(targetType)),
        expression(nullptr) {}

  // 用于 @typeof(expr) 的构造函数
  explicit ReflectionExpr(std::unique_ptr<Expression> expr)
      : kind(TargetKind::Typeof), type(nullptr), expression(std::move(expr)) {}

  NodeType getType() const override { return NodeType::ReflectionExpr; }
  std::string toString() const override;
  std::unique_ptr<Expression> clone() const override;

  TargetKind kind;
  std::unique_ptr<ast::Type> type;        // 当 kind == Type 时有效
  std::unique_ptr<Expression> expression; // 当 kind == Typeof 时有效
};

} // namespace ast
} // namespace c_hat
