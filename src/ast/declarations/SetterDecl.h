#pragma once

#include "Declaration.h"
#include "../expressions/Expression.h"
#include "../others/Parameter.h"
#include <string>
#include <memory>

namespace c_hat {
namespace ast {

// Setter 声明
class SetterDecl : public Declaration {
public:
    SetterDecl(const std::string& specifiers, const std::string& name,
               std::unique_ptr<Parameter> param,
               std::unique_ptr<Node> body,
               std::unique_ptr<Expression> arrowExpr = nullptr)
        : specifiers(specifiers), name(name), param(std::move(param)),
          body(std::move(body)), arrowExpr(std::move(arrowExpr)) {}
    
    NodeType getType() const override { return NodeType::SetterDecl; }
    std::string toString() const override;
    
    std::string specifiers;
    std::string name;
    std::unique_ptr<Parameter> param;
    std::unique_ptr<Node> body;
    std::unique_ptr<Expression> arrowExpr;
};

} // namespace ast
} // namespace c_hat
