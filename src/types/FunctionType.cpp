#include "FunctionType.h"
#include <format>

namespace c_hat {
namespace types {

FunctionType::FunctionType(std::shared_ptr<Type> returnType,
                           std::vector<std::shared_ptr<Type>> parameterTypes)
    : returnType(returnType), parameterTypes(parameterTypes) {}

std::string FunctionType::toString() const {
  std::string paramsStr;
  for (size_t i = 0; i < parameterTypes.size(); i++) {
    if (i > 0) {
      paramsStr += ", ";
    }
    paramsStr += parameterTypes[i]->toString();
  }
  return std::format("func({}) -> {}", paramsStr, returnType->toString());
}

bool FunctionType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isFunction()) {
    return false;
  }
  const auto &otherFunction = static_cast<const FunctionType &>(other);
  if (!returnType->isCompatibleWith(*otherFunction.returnType)) {
    return false;
  }
  if (parameterTypes.size() != otherFunction.parameterTypes.size()) {
    return false;
  }
  for (size_t i = 0; i < parameterTypes.size(); i++) {
    if (!parameterTypes[i]->isCompatibleWith(
            *otherFunction.parameterTypes[i])) {
      return false;
    }
  }
  return true;
}

bool FunctionType::isSubtypeOfImpl(const Type &other) const {
  if (!other.isFunction()) {
    return false;
  }
  const auto &otherFunction = static_cast<const FunctionType &>(other);
  if (!returnType->isSubtypeOf(*otherFunction.returnType)) {
    return false;
  }
  if (parameterTypes.size() != otherFunction.parameterTypes.size()) {
    return false;
  }
  for (size_t i = 0; i < parameterTypes.size(); i++) {
    if (!otherFunction.parameterTypes[i]->isSubtypeOf(*parameterTypes[i])) {
      return false;
    }
  }
  return true;
}

} // namespace types
} // namespace c_hat
