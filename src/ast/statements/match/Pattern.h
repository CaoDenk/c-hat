#pragma once

#include "../../Node.h"

namespace c_hat {
namespace ast {

// 模式
class Pattern : public Node {
public:
    NodeType getType() const override { return NodeType::Pattern; }
    std::string toString() const override { return "Pattern()"; };
};

} // namespace ast
} // namespace c_hat
