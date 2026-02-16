#pragma once

#include "Declaration.h"
#include "EnumMember.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

class EnumDecl : public Declaration {
public:
  EnumDecl(const std::string &specifiers, const std::string &name,
           std::vector<std::unique_ptr<EnumMember>> members)
      : specifiers(specifiers), name(name), members(std::move(members)) {}

  NodeType getType() const override { return NodeType::EnumDecl; }
  std::string toString() const override;

  std::string specifiers;
  std::string name;
  std::vector<std::unique_ptr<EnumMember>> members;
};

} // namespace ast
} // namespace c_hat
