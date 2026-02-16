#pragma once

#include "Expression.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// 成员访问
class MemberExpr : public Expression {
public:
  MemberExpr(std::unique_ptr<Expression> object, const std::string &member)
      : object(std::move(object)), member(member) {}

  NodeType getType() const override { return NodeType::MemberExpr; }
  std::string toString() const override;

  std::unique_ptr<Expression> object;
  std::string member;
};

} // namespace ast
} // namespace c_hat
