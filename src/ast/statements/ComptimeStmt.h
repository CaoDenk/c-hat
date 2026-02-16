#pragma once

#include "Statement.h"
#include <memory>

namespace c_hat {
namespace ast {

// 编译期语句
class ComptimeStmt : public Statement {
public:
  ComptimeStmt(std::unique_ptr<Statement> stmt) : stmt(std::move(stmt)) {}

  NodeType getType() const override { return NodeType::ComptimeStmt; }
  std::string toString() const override;

  std::unique_ptr<Statement> stmt;
};

} // namespace ast
} // namespace c_hat
