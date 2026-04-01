#pragma once

#include "../Node.h"
#include "../others/AttributeApplication.h"
#include <vector>

namespace c_hat {
namespace ast {

class Statement : public Node {
public:
  NodeType getType() const override { return NodeType::Statement; }

  std::vector<std::unique_ptr<AttributeApplication>> attributes;
};

} // namespace ast
} // namespace c_hat
