#pragma once

#include "Expression.h"
#include <string>

namespace c_hat {
namespace ast {

// 标识符
class Identifier : public Expression {
public:
  Identifier(const std::string &name) : name(name) {}

  NodeType getType() const override { return NodeType::Identifier; }
  std::string toString() const override;

  std::string name;
};

} // namespace ast
} // namespace c_hat
