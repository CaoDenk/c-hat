#include "SetterDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string SetterDecl::toString() const {
  std::string specStr = specifiers.empty() ? "" : specifiers + " ";
  std::string paramStr = param ? param->toString() : "";
  return std::format("SetterDecl({}set {}({}))", specStr, name, paramStr);
}

} // namespace ast
} // namespace c_hat
