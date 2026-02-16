#pragma once

#include "Declaration.h"
#include <string>
#include <vector>
#include <memory>

namespace c_hat {
namespace ast {

// 结构体声明
class StructDecl : public Declaration {
public:
    StructDecl(const std::string& specifiers, const std::string& name, std::vector<std::unique_ptr<Node>> members)
        : specifiers(specifiers), name(name), members(std::move(members)) {}
    
    NodeType getType() const override { return NodeType::StructDecl; }
    std::string toString() const override;
    
    std::string specifiers;
    std::string name;
    std::vector<std::unique_ptr<Node>> members;
};

} // namespace ast
} // namespace c_hat
