#pragma once

#include "Declaration.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

// 接口声明
class InterfaceDecl : public Declaration {
public:
  InterfaceDecl(const std::string &specifiers, const std::string &name,
                std::vector<std::string> baseInterfaces,
                std::vector<std::unique_ptr<Node>> members)
      : specifiers(specifiers), name(name),
        baseInterfaces(std::move(baseInterfaces)), members(std::move(members)) {
  }

  NodeType getType() const override { return NodeType::InterfaceDecl; }
  std::string toString() const override;

  std::string specifiers;
  std::string name;
  std::vector<std::string> baseInterfaces; // 父接口列表
  std::vector<std::unique_ptr<Node>> members;
};

} // namespace ast
} // namespace c_hat