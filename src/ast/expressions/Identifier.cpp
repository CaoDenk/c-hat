#include "Identifier.h"
#include <format>

namespace c_hat {
namespace ast {

std::string Identifier::toString() const {
  return std::format("Identifier({})", name);
}

} // namespace ast
} // namespace c_hat
