#pragma once

#include "Declaration.h"
#include "../types/Type.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

// 类型别名声明
class TypeAliasDecl : public Declaration {
public:
  // 用于普通类型别名 (using X = int)
  TypeAliasDecl(const std::string &specifiers, const std::string &name,
                std::unique_ptr<Type> type, bool isTypeSet = false)
      : specifiers(specifiers), name(name), isTypeSet(isTypeSet) {
    if (type) types.push_back(std::move(type));
  }

  // 用于类型集合别名 (using X = A | B | C)
  TypeAliasDecl(const std::string &specifiers, const std::string &name,
                std::vector<std::unique_ptr<Type>> types, bool isTypeSet)
      : specifiers(specifiers), name(name),
        types(std::move(types)), isTypeSet(isTypeSet) {}

  NodeType getType() const override { return NodeType::TypeAliasDecl; }
  std::string toString() const override;

  std::string specifiers;
  std::string name;
  // 类型集合时存储所有类型，普通别名时只有一个
  std::vector<std::unique_ptr<Type>> types;
  bool isTypeSet = false;  // true 表示这是类型集合别名 (using X = A | B | C)
};

} // namespace ast
} // namespace c_hat
