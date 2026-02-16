#pragma once

#include "Declaration.h"
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

// 导入声明
class ImportDecl : public Declaration {
public:
  ImportDecl(const std::string &specifiers, std::vector<std::string> modulePath,
             std::string alias)
      : specifiers(specifiers), modulePath(std::move(modulePath)),
        alias(std::move(alias)) {}

  NodeType getType() const override { return NodeType::ImportDecl; }
  std::string toString() const override;

  std::string specifiers;
  std::vector<std::string> modulePath;
  std::string alias;
};

} // namespace ast
} // namespace c_hat
