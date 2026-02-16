#pragma once

#include "Type.h"

namespace c_hat {
namespace ast {

// 命名类型（如类名、结构体名等）
class NamedType : public Type {
public:
    explicit NamedType(std::string name)
        : name(std::move(name)) {}

    NodeType getType() const override { return NodeType::NamedType; }
    std::string toString() const override { return name; }

    std::string name;
};

} 
} 
