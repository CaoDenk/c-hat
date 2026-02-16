#pragma once

#include "../Node.h"
#include "../expressions/Expression.h"
#include <string>
#include <memory>

namespace c_hat {
namespace ast {

class EnumMember : public Node {
public:
    EnumMember(const std::string& name, std::unique_ptr<Expression> value = nullptr)
        : name(name), value(std::move(value)) {}

    NodeType getType() const override { return NodeType::EnumMember; }
    std::string toString() const override;

    std::string name;
    std::unique_ptr<Expression> value;
};

} // namespace ast
} // namespace c_hat
