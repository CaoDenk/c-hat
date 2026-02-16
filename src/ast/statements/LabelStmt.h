#pragma once

#include "Statement.h"
#include <string>

namespace c_hat {
namespace ast {

class LabelStmt : public Statement {
public:
  std::string label;

  LabelStmt(std::string label) : label(std::move(label)) {}

  NodeType getType() const override { return NodeType::LabelStmt; }
  std::string toString() const override {
    return label + ":";
  }
};

} // namespace ast
} // namespace c_hat
