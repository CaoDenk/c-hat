#pragma once

#include "Declaration.h"
#include "../expressions/Expression.h"
#include <string>
#include <vector>
#include <memory>

namespace c_hat {
namespace ast {

// 函数声明
class FunctionDecl : public Declaration {
public:
    FunctionDecl(const std::string& specifiers, const std::string& name,
                 std::vector<std::unique_ptr<Node>> templateParams,
                 std::vector<std::unique_ptr<Node>> params,
                 std::unique_ptr<Node> returnType,
                 std::unique_ptr<Node> whereClause,
                 std::unique_ptr<Node> requiresClause,
                 std::unique_ptr<Node> body,
                 std::unique_ptr<Expression> arrowExpr = nullptr)
        : specifiers(specifiers), name(name), templateParams(std::move(templateParams)),
          params(std::move(params)), returnType(std::move(returnType)),
          whereClause(std::move(whereClause)), requiresClause(std::move(requiresClause)),
          body(std::move(body)), arrowExpr(std::move(arrowExpr)) {}
    
    NodeType getType() const override { return NodeType::FunctionDecl; }
    std::string toString() const override;
    
    std::string specifiers;
    std::string name;
    std::vector<std::unique_ptr<Node>> templateParams;
    std::vector<std::unique_ptr<Node>> params;
    std::unique_ptr<Node> returnType;
    std::unique_ptr<Node> whereClause;
    std::unique_ptr<Node> requiresClause;
    std::unique_ptr<Node> body;
    std::unique_ptr<Expression> arrowExpr;
};

} // namespace ast
} // namespace c_hat
