#include "GenericType.h"
#include <format>

namespace c_hat {
namespace ast {

std::string GenericType::toString() const {
  std::string argsStr = "";
  for (size_t i = 0; i < arguments.size(); i++) {
    if (i > 0)
      argsStr += ", ";
    argsStr += arguments[i]->toString();
  }
  return std::format("GenericType({}<{}>)", name, argsStr);
}

} // namespace ast
} // namespace c_hat
