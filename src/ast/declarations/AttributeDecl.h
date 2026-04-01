#pragma once

#include "../expressions/Expression.h"
#include "../types/Type.h"
#include "Declaration.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

class AttributeField {
public:
  std::string name;
  std::unique_ptr<Type> type;
  std::unique_ptr<Expression> defaultValue;

  AttributeField(const std::string &name, std::unique_ptr<Type> type,
                 std::unique_ptr<Expression> defaultValue = nullptr)
      : name(name), type(std::move(type)),
        defaultValue(std::move(defaultValue)) {}
};

class AttributeDecl : public Declaration {
public:
  AttributeDecl(const std::string &name) : name(name) {}

  NodeType getType() const override { return NodeType::AttributeDecl; }
  std::string toString() const override;

  void addField(std::unique_ptr<AttributeField> field) {
    fields.push_back(std::move(field));
  }

  std::string name;
  std::vector<std::unique_ptr<AttributeField>> fields;
};

} // namespace ast
} // namespace c_hat
