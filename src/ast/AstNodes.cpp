#include "AstNodes.h"
#include <format>
#include <sstream>

namespace c_hat {
namespace ast {

// ExportDecl::toString
std::string ExportDecl::toString() const {
  std::string specStr = specifiers.empty() ? "" : specifiers + " ";
  return std::format("ExportDecl({}{})", specStr, decl->toString());
}

// TypeAliasDecl::toString
std::string TypeAliasDecl::toString() const {
  std::string specStr = specifiers.empty() ? "" : specifiers + " ";
  return std::format("TypeAliasDecl({}{}, {})", specStr, name,
                     type->toString());
}

} // namespace ast
} // namespace c_hat
