#pragma once

#include "Statement.h"
#include "../expressions/Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// 条件语句
class IfStmt : public Statement {
public:
    IfStmt(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> thenBranch, std::unique_ptr<Statement> elseBranch = nullptr)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    
    NodeType getType() const override { return NodeType::IfStmt; }
    std::string toString() const override;
    
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
};

} // namespace ast
} // namespace c_hat
