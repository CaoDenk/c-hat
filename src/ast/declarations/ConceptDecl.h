#pragma once

#include "Declaration.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

class ConceptDecl : public Declaration {
public:
  ConceptDecl(const std::string &name,
              std::vector<std::unique_ptr<Node>> templateParams,
              std::vector<std::unique_ptr<Node>> constraints)
      : name(name), templateParams(std::move(templateParams)),
        constraints(std::move(constraints)) {}

  NodeType getType() const override { return NodeType::ConceptDecl; }
  std::string toString() const override;

  std::string name;
  std::vector<std::unique_ptr<Node>> templateParams;
  std::vector<std::unique_ptr<Node>> constraints;
};

} // namespace ast
} // namespace c_hat
