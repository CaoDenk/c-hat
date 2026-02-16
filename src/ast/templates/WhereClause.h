#pragma once

#include "../Node.h"
#include "../types/Type.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

// where子句
class WhereClause : public Node {
public:
  struct Constraint {
    std::string typeParam;
    std::unique_ptr<Type> constraint;
  };

  WhereClause(std::vector<Constraint> constraints)
      : constraints(std::move(constraints)) {}

  NodeType getType() const override { return NodeType::WhereClause; }
  std::string toString() const override;

  std::vector<Constraint> constraints;
};

} // namespace ast
} // namespace c_hat
