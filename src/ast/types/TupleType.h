#pragma once

#include "Type.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace ast {

// 元组类型
class TupleType : public Type {
public:
    explicit TupleType(std::vector<std::unique_ptr<Type>> elementTypes)
        : elementTypes(std::move(elementTypes)) {}
    
    NodeType getType() const override { return NodeType::TupleType; }
    std::string toString() const override;
    std::unique_ptr<Type> clone() const override {
        std::vector<std::unique_ptr<Type>> clonedElementTypes;
        for (const auto &elemType : elementTypes) {
            clonedElementTypes.push_back(elemType->clone());
        }
        return std::make_unique<TupleType>(std::move(clonedElementTypes));
    }
    
    std::vector<std::unique_ptr<Type>> elementTypes;
};

} // namespace ast
} // namespace c_hat
