#pragma once

#include "Expression.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// 成员访问
class MemberExpr : public Expression {
public:
  MemberExpr(std::unique_ptr<Expression> object, const std::string &member,
             bool isPointerMember = false)
      : object(std::move(object)), member(member),
        isPointerMember(isPointerMember) {}

  NodeType getType() const override { return NodeType::MemberExpr; }
  std::string toString() const override;
  std::unique_ptr<Expression> clone() const override {
    auto cloned = std::make_unique<MemberExpr>(object->clone(), member, isPointerMember);
    cloned->structName = structName;
    cloned->memberIndex = memberIndex;
    return cloned;
  }

  std::unique_ptr<Expression> object;
  std::string member;
  bool isPointerMember;
  std::string structName;
  unsigned memberIndex = 0;
};

} // namespace ast
} // namespace c_hat
