#pragma once

#include "Statement.h"
#include "ForStmt.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// 编译期 for 循环 - 在编译期完全展开
class ComptimeForStmt : public Statement {
public:
  ComptimeForStmt(std::unique_ptr<ForStmt> forStmt)
      : forStmt(std::move(forStmt)) {}

  NodeType getType() const override { return NodeType::ComptimeForStmt; }
  std::string toString() const override {
    return "comptime " + (forStmt ? forStmt->toString() : "for {}");
  }

  std::unique_ptr<ForStmt> forStmt;
};

} // namespace ast
} // namespace c_hat
