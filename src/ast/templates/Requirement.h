#pragma once

#include "../Node.h"
#include "../expressions/Expression.h"
#include "../types/Type.h"
#include <memory>

namespace c_hat {
namespace ast {

// 需求
class Requirement : public Node {
public:
  enum class RequirementType {
    Compound, // 复合要求
    Typename  // 类型要求
  };

  Requirement(RequirementType type, std::unique_ptr<Expression> expr,
              std::unique_ptr<Type> typeSpec)
      : type(type), expr(std::move(expr)), typeSpec(std::move(typeSpec)) {}

  Requirement(RequirementType type, std::unique_ptr<Type> typeSpec)
      : type(type), typeSpec(std::move(typeSpec)) {}

  NodeType getType() const override { return NodeType::Requirement; }
  std::string toString() const override;

  RequirementType type;
  std::unique_ptr<Expression> expr; // 复合要求的表达式
  std::unique_ptr<Type> typeSpec;   // 类型说明符
};

} // namespace ast
} // namespace c_hat
