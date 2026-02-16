#include "Parameter.h"
#include <format>

namespace c_hat {
namespace ast {

std::string Parameter::toString() const {
  std::string defaultValueStr = "";
  if (defaultValue) {
    defaultValueStr = std::format(" = {}", defaultValue->toString());
  }
  return std::format("Parameter({}: {}{})", name, type->toString(),
                     defaultValueStr);
}

} // namespace ast
} // namespace c_hat
