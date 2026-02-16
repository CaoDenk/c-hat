#include "ClassDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ClassDecl::toString() const {
  std::string membersStr = "";
  for (size_t i = 0; i < members.size(); i++) {
    if (i > 0)
      membersStr += ", ";
    membersStr += members[i]->toString();
  }
  return std::format("ClassDecl({} {} extends {}, [{}])", specifiers, name,
                     baseClass, membersStr);
}

} // namespace ast
} // namespace c_hat
