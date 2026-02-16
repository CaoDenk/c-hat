#include "GetterDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string GetterDecl::toString() const {
  std::string returnTypeStr = "";
  if (returnType) {
    returnTypeStr = std::format(" -> {}", returnType->toString());
  }
  std::string specStr = specifiers.empty() ? "" : specifiers + " ";
  return std::format("GetterDecl({}get {}{})", specStr, name, returnTypeStr);
}

} // namespace ast
} // namespace c_hat
