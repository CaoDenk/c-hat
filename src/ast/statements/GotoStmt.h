#pragma once

#include "Statement.h"
#include <string>

namespace c_hat {
namespace ast {

class GotoStmt : public Statement {
public:
  std::string label;

  GotoStmt(std::string label) : label(std::move(label)) {}

  NodeType getType() const override { return NodeType::GotoStmt; }
  std::string toString() const override {
    return "goto " + label + ";";
  }
};

} // namespace ast
} // namespace c_hat
