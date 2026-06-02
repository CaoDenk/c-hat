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
  std::string typeStr;
  if (types.empty()) {
    typeStr = "∅";
  } else if (isTypeSet) {
    for (size_t i = 0; i < types.size(); ++i) {
      if (i > 0) typeStr += " | ";
      typeStr += types[i]->toString();
    }
  } else {
    typeStr = types[0]->toString();
  }
  return std::format("TypeAliasDecl({}{}, {})", specStr, name, typeStr);
}

// TupleDestructuringDecl::toString
std::string TupleDestructuringDecl::toString() const {
  std::string specStr = specifiers.empty() ? "" : specifiers + " ";
  std::string kindStr;
  switch (kind) {
  case VariableKind::Var:
    kindStr = "var ";
    break;
  case VariableKind::Let:
    kindStr = "let ";
    break;
  default:
    kindStr = "";
    break;
  }

  std::string namesStr = "(";
  for (size_t i = 0; i < names.size(); ++i) {
    if (i > 0) {
      namesStr += ", ";
    }
    namesStr += names[i];
  }
  namesStr += ")";

  return std::format("TupleDestructuringDecl({}{}{})", specStr, kindStr,
                     namesStr);
}

} // namespace ast
} // namespace c_hat
