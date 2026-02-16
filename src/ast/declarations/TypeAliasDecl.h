#pragma once

#include "Declaration.h"
#include "../types/Type.h"

namespace c_hat {
namespace ast {

// 类型别名声明
class TypeAliasDecl : public Declaration {
public:
  TypeAliasDecl(const std::string &specifiers, const std::string &name, std::unique_ptr<Type> type)
      : specifiers(specifiers), name(name), type(std::move(type)) {}

  NodeType getType() const override { return NodeType::TypeAliasDecl; }
  std::string toString() const override;

  std::string specifiers;
  std::string name;
  std::unique_ptr<Type> type;
};

} // namespace ast
} // namespace c_hat
