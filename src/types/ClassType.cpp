#include "ClassType.h"

namespace c_hat {
namespace types {

ClassType::ClassType(std::string name) : name(name) {}

std::string ClassType::toString() const { return name; }

bool ClassType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isClass()) {
    return false;
  }
  const auto &otherClass = static_cast<const ClassType &>(other);
  return name == otherClass.name;
}

bool ClassType::isSubtypeOfImpl(const Type &other) const {
  if (isCompatibleWithImpl(other)) {
    return true;
  }
  if (!other.isClass()) {
    return false;
  }
  const auto &otherClass = static_cast<const ClassType &>(other);
  for (const auto &baseClass : baseClasses) {
    if (baseClass->isSubtypeOf(otherClass)) {
      return true;
    }
  }
  return false;
}

void ClassType::addBaseClass(std::shared_ptr<ClassType> baseClass) {
  baseClasses.push_back(baseClass);
}

} // namespace types
} // namespace c_hat
