#include "ModuleDecl.h"
#include <sstream>

namespace c_hat {
namespace ast {

std::string ModuleDecl::toString() const {
  std::ostringstream oss;
  oss << "module ";
  for (size_t i = 0; i < modulePath.size(); ++i) {
    if (i > 0) oss << ".";
    oss << modulePath[i];
  }
  return oss.str();
}

} // namespace ast
} // namespace c_hat
