#pragma once

#include "../Node.h"
#include "../declarations/Declaration.h"
#include <vector>
#include <memory>

namespace c_hat {
namespace ast {

// 程序节点
class Program : public Node {
public:
    Program(std::vector<std::unique_ptr<Declaration>> declarations)
        : declarations(std::move(declarations)) {}
    
    NodeType getType() const override { return NodeType::Program; }
    std::string toString() const override;
    
    std::vector<std::unique_ptr<Declaration>> declarations;
};

} // namespace ast
} // namespace c_hat
