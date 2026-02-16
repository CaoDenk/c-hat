#pragma once

#include "Statement.h"

namespace c_hat {
namespace ast {

class BreakStmt : public Statement {
public:
  NodeType getType() const override { return NodeType::BreakStmt; }
  std::string toString() const override { return "break;"; }
};

} // namespace ast
} // namespace c_hat
