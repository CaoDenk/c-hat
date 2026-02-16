#include "EnumMember.h"
#include <format>

namespace c_hat {
namespace ast {

std::string EnumMember::toString() const {
  if (value) {
    return std::format("EnumMember({}, {})", name, value->toString());
  }
  return std::format("EnumMember({})", name);
}

} // namespace ast
} // namespace c_hat
