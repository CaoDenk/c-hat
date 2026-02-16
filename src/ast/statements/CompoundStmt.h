#pragma once

#include "Statement.h"
#include <vector>
#include <memory>

namespace c_hat {
namespace ast {

// 复合语句
class CompoundStmt : public Statement {
public:
  CompoundStmt() = default;
  CompoundStmt(std::vector<std::unique_ptr<Node>> statements)
      : statements(std::move(statements)) {}

  NodeType getType() const override { return NodeType::CompoundStmt; }
  std::string toString() const override;

  std::vector<std::unique_ptr<Node>> statements;
};

} // namespace ast
} // namespace c_hat
