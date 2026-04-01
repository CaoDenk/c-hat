#pragma once

#include "../Node.h"
#include "../others/AttributeApplication.h"
#include <vector>

namespace c_hat {
namespace ast {

class Declaration : public Node {
public:
  NodeType getType() const override { return NodeType::Declaration; }

  std::vector<std::unique_ptr<AttributeApplication>> attributes;
};

} // namespace ast
} // namespace c_hat
