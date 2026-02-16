#pragma once

#include "Declaration.h"

namespace c_hat {
namespace ast {

// 导出声明
class ExportDecl : public Declaration {
public:
  ExportDecl(const std::string &specifiers, std::unique_ptr<Declaration> decl)
      : specifiers(specifiers), decl(std::move(decl)) {}

  NodeType getType() const override { return NodeType::ExportDecl; }
  std::string toString() const override;

  std::string specifiers;
  std::unique_ptr<Declaration> decl;
};

} // namespace ast
} // namespace c_hat
