#include "ImportDecl.h"
#include <sstream>

namespace c_hat {
namespace ast {

std::string ImportDecl::toString() const {
  std::ostringstream oss;
  if (!specifiers.empty()) {
    oss << specifiers << " ";
  }
  oss << "import ";
  for (size_t i = 0; i < modulePath.size(); ++i) {
    if (i > 0) oss << ".";
    oss << modulePath[i];
  }
  if (!alias.empty()) {
    oss << " as " << alias;
  }
  return oss.str();
}

} // namespace ast
} // namespace c_hat
