#pragma once

#include "../../Node.h"
#include "../../expressions/Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// 模式
class Pattern : public Node {
public:
    bool isDefault = false;
    std::unique_ptr<Expression> value;

    Pattern() = default;
    explicit Pattern(std::unique_ptr<Expression> value)
        : value(std::move(value)) {}

    NodeType getType() const override { return NodeType::Pattern; }
    std::string toString() const override { return "Pattern()"; };
};

} // namespace ast
} // namespace c_hat
