#pragma once

#include "Expression.h"
#include <string>

namespace c_hat {
namespace ast {

// 字面量
class Literal : public Expression {
public:
  enum class Type { Integer, Floating, Boolean, Character, String, Null };

  Literal(Type type, const std::string &value) : type(type), value(value) {}

  NodeType getType() const override { return NodeType::Literal; }
  std::string toString() const override;

  Type type;
  std::string value;
};

} // namespace ast
} // namespace c_hat
