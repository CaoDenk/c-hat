#pragma once

#include "../Node.h"

namespace c_hat {
namespace ast {

// 表达式节点
class Expression : public Node {
public:
  NodeType getType() const override { return NodeType::Expression; }
};

} // namespace ast
} // namespace c_hat
