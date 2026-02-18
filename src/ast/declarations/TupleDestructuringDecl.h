#pragma once

#include "../expressions/Expression.h"
#include "Declaration.h"
#include "VariableDecl.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

// 元组解构声明
class TupleDestructuringDecl : public Declaration {
public:
  TupleDestructuringDecl(const std::string &specifiers, bool isLate,
                         VariableKind kind, std::vector<std::string> names,
                         std::unique_ptr<Expression> initializer)
      : specifiers(specifiers), isLate(isLate), kind(kind),
        names(std::move(names)), initializer(std::move(initializer)) {}

  NodeType getType() const override { return NodeType::TupleDestructuringDecl; }
  std::string toString() const override;

  std::string specifiers;
  bool isLate;
  VariableKind kind;
  std::vector<std::string> names;
  std::unique_ptr<Expression> initializer;
};

} // namespace ast
} // namespace c_hat
