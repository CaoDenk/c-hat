#include "ReadonlyType.h"

namespace c_hat {
namespace ast {

ReadonlyType::ReadonlyType(std::unique_ptr<Type> baseType)
  : baseType(std::move(baseType)) {}

std::string ReadonlyType::toString() const {
  return baseType->toString() + "!";
}

} // namespace ast
} // namespace c_hat
