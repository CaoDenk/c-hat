#pragma once

#include "Expression.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

class Identifier : public Expression {
public:
  Identifier(const std::string &name) : name(name) {}

  Identifier(const std::string &name,
             std::vector<std::unique_ptr<Node>> templateArgs)
      : name(name), templateArgs(std::move(templateArgs)) {}

  NodeType getType() const override { return NodeType::Identifier; }
  std::string toString() const override;
  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<Identifier>(name);
  }

  std::string name;
  std::vector<std::unique_ptr<Node>> templateArgs;
};

} // namespace ast
} // namespace c_hat
