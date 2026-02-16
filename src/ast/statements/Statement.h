#pragma once

#include "../Node.h"

namespace c_hat {
namespace ast {

// 语句节点
class Statement : public Node {
public:
  NodeType getType() const override { return NodeType::Statement; }
};

} // namespace ast
} // namespace c_hat
