#pragma once

#include "Type.h"
#include "../expressions/Expression.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace ast {

// 矩形数组类型（固定大小，如 int[2, 3]
class RectangularArrayType : public Type {
public:
    RectangularArrayType(std::unique_ptr<Type> baseType, std::vector<std::unique_ptr<Expression>> sizes)
        : baseType(std::move(baseType)), sizes(std::move(sizes)) {}
    
    NodeType getType() const override { return NodeType::RectangularArrayType; }
    std::string toString() const override;
    
    std::unique_ptr<Type> baseType;
    std::vector<std::unique_ptr<Expression>> sizes;
};

} // namespace ast
} // namespace c_hat
