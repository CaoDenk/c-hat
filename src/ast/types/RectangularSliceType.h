#pragma once

#include "Type.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace ast {

// 矩形切片类型（多维切片，如 int[,] 或 int[,,]）
class RectangularSliceType : public Type {
public:
    RectangularSliceType(std::unique_ptr<Type> baseType, int rank)
        : baseType(std::move(baseType)), rank(rank) {}
    
    NodeType getType() const override { return NodeType::RectangularSliceType; }
    std::string toString() const override;
    
    std::unique_ptr<Type> baseType;
    int rank; // 维度数量，如 int[,] 是 2 维
};

} // namespace ast
} // namespace c_hat
