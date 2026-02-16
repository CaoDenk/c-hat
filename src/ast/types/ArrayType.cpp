#include "ArrayType.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ArrayType::toString() const {
  return std::format("ArrayType({}, {})", baseType->toString(),
                     size->toString());
}

} // namespace ast
} // namespace c_hat
