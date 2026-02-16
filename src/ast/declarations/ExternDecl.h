#pragma once

#include "Declaration.h"
#include <string>
#include <vector>
#include <memory>

namespace c_hat {
namespace ast {

// 外部声明块
class ExternDecl : public Declaration {
public:
    ExternDecl(std::string abi,
                  std::vector<std::unique_ptr<Declaration>> declarations)
        : abi(std::move(abi)),
          declarations(std::move(declarations)) {}

    NodeType getType() const override { return NodeType::ExternDecl; }
    std::string toString() const override;

    std::string abi;
    std::vector<std::unique_ptr<Declaration>> declarations;
};

} // namespace ast
} // namespace c_hat
