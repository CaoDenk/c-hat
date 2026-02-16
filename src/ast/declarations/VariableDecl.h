#pragma once

#include "../expressions/Expression.h"
#include "Declaration.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

enum class VariableKind { Var, Let, Explicit };

// 变量声明
class VariableDecl : public Declaration {
public:
  VariableDecl(const std::string &specifiers, bool isLate, VariableKind kind,
               std::unique_ptr<Node> type, const std::string &name,
               std::unique_ptr<Expression> initializer = nullptr,
               bool isConst = false)
      : specifiers(specifiers), isLate(isLate), kind(kind), isConst(isConst),
        type(std::move(type)), name(name), initializer(std::move(initializer)) {
  }

  NodeType getType() const override { return NodeType::VariableDecl; }
  std::string toString() const override;

  std::string specifiers;
  bool isLate;
  VariableKind kind;
  bool isConst;
  std::unique_ptr<Node> type;
  std::string name;
  std::unique_ptr<Expression> initializer;
};

} // namespace ast
} // namespace c_hat
