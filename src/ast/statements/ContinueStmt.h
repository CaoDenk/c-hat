#pragma once

#include "Statement.h"

namespace c_hat {
namespace ast {

// continue语句
class ContinueStmt : public Statement {
public:
    NodeType getType() const override { return NodeType::ContinueStmt; }
    std::string toString() const override { return "continue;"; }
};

} // namespace ast
} // namespace c_hat
