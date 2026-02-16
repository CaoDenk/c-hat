#include "TupleType.h"
#include <format>

namespace c_hat {
namespace types {

TupleType::TupleType(std::vector<std::shared_ptr<Type>> elementTypes)
    : elementTypes(std::move(elementTypes)) {}

std::string TupleType::toString() const {
  std::string result = "(";
  for (size_t i = 0; i < elementTypes.size(); i++) {
    if (i > 0) {
      result += ", ";
    }
    result += elementTypes[i]->toString();
  }
  result += ")";
  return result;
}

bool TupleType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isTuple()) {
    return false;
  }
  const auto &otherTuple = static_cast<const TupleType &>(other);
  if (elementTypes.size() != otherTuple.elementTypes.size()) {
    return false;
  }
  for (size_t i = 0; i < elementTypes.size(); i++) {
    if (!elementTypes[i]->isCompatibleWith(*otherTuple.elementTypes[i])) {
      return false;
    }
  }
  return true;
}

bool TupleType::isSubtypeOfImpl(const Type &other) const {
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
