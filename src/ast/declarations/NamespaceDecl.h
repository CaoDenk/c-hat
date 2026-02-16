#pragma once

#include "Declaration.h"

namespace c_hat {
namespace ast {

// 命名空间声明
class NamespaceDecl : public Declaration {
public:
    NamespaceDecl(const std::string& name, std::vector<std::unique_ptr<Node>> members)
        : name(name), members(std::move(members)) {}
    
    NodeType getType() const override { return NodeType::NamespaceDecl; }
    std::string toString() const override;
    
    std::string name;
    std::vector<std::unique_ptr<Node>> members;
};

} // namespace ast
} // namespace c_hat
