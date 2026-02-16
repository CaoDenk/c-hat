#include "Literal.h"
#include <format>

namespace c_hat {
namespace ast {

std::string Literal::toString() const {
  std::string typeStr;
  switch (type) {
  case Type::Integer:
    typeStr = "Integer";
    break;
  case Type::Floating:
    typeStr = "Floating";
    break;
  case Type::String:
    typeStr = "String";
    break;
  case Type::Character:
    typeStr = "Character";
    break;
  case Type::Boolean:
    typeStr = "Boolean";
    break;
  case Type::Null:
    typeStr = "Null";
    break;
  default:
    typeStr = "Unknown";
    break;
  }
  return std::format("Literal({}, {})", typeStr, value);
}

} // namespace ast
} // namespace c_hat
