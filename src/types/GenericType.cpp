#include "GenericType.h"
#include <format>

namespace c_hat {
namespace types {

GenericType::GenericType(std::string name,
                         std::vector<std::shared_ptr<Type>> typeArguments)
    : name(name), typeArguments(typeArguments) {}

std::string GenericType::toString() const {
  std::string argsStr;
  for (size_t i = 0; i < typeArguments.size(); i++) {
    if (i > 0) {
      argsStr += ", ";
    }
    argsStr += typeArguments[i]->toString();
  }
  return std::format("{}<{}>", name, argsStr);
}

bool GenericType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isGeneric()) {
    return false;
  }
  const auto &otherGeneric = static_cast<const GenericType &>(other);
  if (name != otherGeneric.name ||
      typeArguments.size() != otherGeneric.typeArguments.size()) {
    return false;
  }
  for (size_t i = 0; i < typeArguments.size(); i++) {
    if (!typeArguments[i]->isCompatibleWith(*otherGeneric.typeArguments[i])) {
      return false;
    }
  }
  return true;
}

bool GenericType::isSubtypeOfImpl(const Type &other) const {
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
