#include "PointerType.h"
#include <format>

namespace c_hat {
namespace ast {

std::string PointerType::toString() const {
  return std::format("PointerType({}{})", baseType->toString(),
                     isNullable ? "?" : "");
}

} // namespace ast
} // namespace c_hat
