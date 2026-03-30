#include "InterfaceDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string InterfaceDecl::toString() const {
  std::string baseInterfacesStr = "";
  for (size_t i = 0; i < baseInterfaces.size(); i++) {
    if (i > 0)
      baseInterfacesStr += ", ";
    baseInterfacesStr += baseInterfaces[i];
  }

  std::string membersStr = "";
  for (size_t i = 0; i < members.size(); i++) {
    if (i > 0)
      membersStr += ", ";
    membersStr += members[i]->toString();
  }

  return std::format("InterfaceDecl({} {} extends [{}], [{}])", specifiers,
                     name, baseInterfacesStr, membersStr);
}

} // namespace ast
} // namespace c_hat
