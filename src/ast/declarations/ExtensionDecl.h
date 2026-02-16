#pragma once

#include "Declaration.h"
#include "../types/Type.h"
#include <string>
#include <vector>
#include <memory>

namespace c_hat {
namespace ast {

// 扩展声明
class ExtensionDecl : public Declaration {
public:
    ExtensionDecl(std::unique_ptr<Type> extendedType,
                  std::vector<std::unique_ptr<Node>> members)
        : extendedType(std::move(extendedType)),
          members(std::move(members)) {}

    NodeType getType() const override { return NodeType::ExtensionDecl; }
    std::string toString() const override;

    std::unique_ptr<Type> extendedType;
    std::vector<std::unique_ptr<Node>> members;
};

} // namespace ast
} // namespace c_hat
