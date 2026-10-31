#pragma once

#include "Declaration.h"
#include "../expressions/Expression.h"
#include "../templates/WhereClause.h"
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
                 std::unique_ptr<WhereClause> whereClause,
                 std::unique_ptr<Node> requiresClause,
                 std::unique_ptr<Node> body,
                 std::unique_ptr<Expression> arrowExpr = nullptr,
                 bool isImmutable = false,
                 std::unique_ptr<Expression> baseCall = nullptr,
                 bool isVariadic = false,
                 bool isStatic = false)
        : specifiers(specifiers), name(name), templateParams(std::move(templateParams)),
          params(std::move(params)), returnType(std::move(returnType)),
          whereClause(std::move(whereClause)), requiresClause(std::move(requiresClause)),
          body(std::move(body)), arrowExpr(std::move(arrowExpr)), isImmutable(isImmutable),
          baseCall(std::move(baseCall)), isVariadic(isVariadic), isStatic(isStatic) {}
    
    NodeType getType() const override { return NodeType::FunctionDecl; }
    std::string toString() const override;
    
    std::string specifiers;
    std::string name;
    std::vector<std::unique_ptr<Node>> templateParams;
    std::vector<std::unique_ptr<Node>> params;
    std::unique_ptr<Node> returnType;
    std::unique_ptr<WhereClause> whereClause;
    std::unique_ptr<Node> requiresClause;
    std::unique_ptr<Node> body;
    std::unique_ptr<Expression> arrowExpr;
    bool isImmutable;
    std::unique_ptr<Expression> baseCall;
    bool isVariadic;
    bool isStatic;
};

} // namespace ast
} // namespace c_hat
