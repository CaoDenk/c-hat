#pragma once

#include "Statement.h"
#include "../expressions/Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// yield语句
class YieldStmt : public Statement {
public:
    YieldStmt(std::unique_ptr<Expression> expr = nullptr)
        : expr(std::move(expr)) {}
    
    NodeType getType() const override { return NodeType::YieldStmt; }
    std::string toString() const override;
    
    std::unique_ptr<Expression> expr;
};

} // namespace ast
} // namespace c_hat
