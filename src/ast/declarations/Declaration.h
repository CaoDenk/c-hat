#pragma once

#include "../Node.h"

namespace c_hat {
namespace ast {

// 声明节点
class Declaration : public Node {
public:
  NodeType getType() const override { return NodeType::Declaration; }
};

} // namespace ast
} // namespace c_hat
