#pragma once

#include "../Statement.h"
#include "CatchStmt.h"
#include <vector>
#include <memory>

namespace c_hat {
namespace ast {

// try语句
class TryStmt : public Statement {
public:
  TryStmt(std::unique_ptr<Statement> tryBlock,
          std::vector<std::unique_ptr<CatchStmt>> catchStmts)
      : tryBlock(std::move(tryBlock)), catchStmts(std::move(catchStmts)) {}

  NodeType getType() const override { return NodeType::TryStmt; }
  std::string toString() const override;

  std::unique_ptr<Statement> tryBlock;
  std::vector<std::unique_ptr<CatchStmt>> catchStmts;
};

} // namespace ast
} // namespace c_hat
