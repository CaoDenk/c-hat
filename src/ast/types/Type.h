#pragma once

#include "../Node.h"

namespace c_hat {
namespace ast {

// 类型节点
class Type : public Node {
public:
    NodeType getType() const override { return NodeType::Type; }
};

} // namespace ast
} // namespace c_hat
