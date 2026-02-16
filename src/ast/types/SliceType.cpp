#include "SliceType.h"
#include <format>

namespace c_hat {
namespace ast {

std::string SliceType::toString() const {
  return std::format("SliceType({})", baseType->toString());
}

} // namespace ast
} // namespace c_hat
