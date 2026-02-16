#pragma once

#include "Declaration.h"
#include "../expressions/Expression.h"
#include <string>
#include <memory>

namespace c_hat {
namespace ast {

// Getter 声明
class GetterDecl : public Declaration {
public:
    GetterDecl(const std::string& specifiers, const std::string& name,
               std::unique_ptr<Node> returnType,
               std::unique_ptr<Node> body,
               std::unique_ptr<Expression> arrowExpr = nullptr)
        : specifiers(specifiers), name(name), returnType(std::move(returnType)),
          body(std::move(body)), arrowExpr(std::move(arrowExpr)) {}
    
    NodeType getType() const override { return NodeType::GetterDecl; }
    std::string toString() const override;
    
    std::string specifiers;
    std::string name;
    std::unique_ptr<Node> returnType;
    std::unique_ptr<Node> body;
    std::unique_ptr<Expression> arrowExpr;
};

} // namespace ast
} // namespace c_hat
