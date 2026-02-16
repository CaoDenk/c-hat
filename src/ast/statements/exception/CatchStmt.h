#pragma once

#include "../../others/Parameter.h"
#include "../Statement.h"
#include <memory>


namespace c_hat {
namespace ast {

// catch语句
class CatchStmt : public Statement {
public:
  CatchStmt(std::unique_ptr<Parameter> param, std::unique_ptr<Statement> body)
      : param(std::move(param)), body(std::move(body)) {}

  NodeType getType() const override { return NodeType::CatchStmt; }
  std::string toString() const override;

  std::unique_ptr<Parameter> param;
  std::unique_ptr<Statement> body;
};

} // namespace ast
} // namespace c_hat
