#pragma once

#include "Expression.h"
#include <string>

namespace c_hat {
namespace ast {

class BuiltinVarExpr : public Expression {
public:
  std::string name;

  BuiltinVarExpr(const std::string &name) : name(name) {}

  NodeType getType() const override { return NodeType::BuiltinVarExpr; }
  std::string toString() const override { return "BuiltinVar(" + name + ")"; }
  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<BuiltinVarExpr>(name);
  }
};

} // namespace ast
} // namespace c_hat
