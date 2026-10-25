#pragma once

#include "Declaration.h"
#include "../templates/WhereClause.h"
#include <string>
#include <vector>
#include <memory>

namespace c_hat {
namespace ast {

// 类声明
class ClassDecl : public Declaration {
public:
    ClassDecl(const std::string& specifiers, const std::string& name,
              std::vector<std::unique_ptr<Node>> templateParams, const std::string& baseClass, 
              std::vector<std::string> baseClasses, std::vector<std::string> interfaces, std::vector<std::unique_ptr<Node>> members,
              std::unique_ptr<WhereClause> whereClause = nullptr)
        : specifiers(specifiers), name(name), templateParams(std::move(templateParams)),
          baseClass(baseClass), baseClasses(std::move(baseClasses)), interfaces(std::move(interfaces)), members(std::move(members)),
          whereClause(std::move(whereClause)) {}
    
    NodeType getType() const override { return NodeType::ClassDecl; }
    std::string toString() const override;
    
    std::string specifiers;
    std::string name;
    std::vector<std::unique_ptr<Node>> templateParams;
    std::string baseClass;
    std::vector<std::string> baseClasses;
    std::vector<std::string> interfaces;
    std::vector<std::unique_ptr<Node>> members;
    std::unique_ptr<WhereClause> whereClause;
};

} // namespace ast
} // namespace c_hat
