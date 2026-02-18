#pragma once

#include "../declarations/TupleDestructuringDecl.h"
#include "Statement.h"

namespace c_hat {
namespace ast {

class TupleDestructuringStmt : public Statement {
public:
  std::unique_ptr<TupleDestructuringDecl> declaration;

  TupleDestructuringStmt(std::unique_ptr<TupleDestructuringDecl> decl)
      : declaration(std::move(decl)) {}

  NodeType getType() const override { return NodeType::TupleDestructuringDecl; }

  std::string toString() const override {
    return declaration->toString();
  }
};

} // namespace ast
} // namespace c_hat
