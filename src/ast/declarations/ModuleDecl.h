#pragma once

#include "../NodeType.h"
#include "Declaration.h"
#include <string>
#include <vector>


namespace c_hat {
namespace ast {

// 模块声明
class ModuleDecl : public Declaration {
public:
  ModuleDecl(std::vector<std::string> modulePath)
      : modulePath(std::move(modulePath)) {}

  NodeType getType() const override { return NodeType::ModuleDecl; }
  std::string toString() const override;

  std::vector<std::string> modulePath;
};

} // namespace ast
} // namespace c_hat
