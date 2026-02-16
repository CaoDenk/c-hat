#pragma once

#include "Statement.h"
#include "../declarations/VariableDecl.h"

namespace c_hat {
namespace ast {

class VariableStmt : public Statement {
public:
  std::unique_ptr<VariableDecl> declaration;

  VariableStmt(std::unique_ptr<VariableDecl> decl)
      : declaration(std::move(decl)) {}

  NodeType getType() const override { return NodeType::VariableDecl; }

  std::string toString() const override {
    return declaration->toString();
  }
};

} 
} 
