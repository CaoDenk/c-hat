#include "AttributeDecl.h"

namespace c_hat {
namespace ast {

std::string AttributeDecl::toString() const {
  std::string result = "attribute " + name + " {";
  for (size_t i = 0; i < fields.size(); ++i) {
    const auto &field = fields[i];
    if (i > 0) {
      result += ", ";
    }
    result += field->name;
    if (field->type) {
      result += ": " + field->type->toString();
    }
    if (field->defaultValue) {
      result += " = " + field->defaultValue->toString();
    }
  }
  result += "}";
  return result;
}

} // namespace ast
} // namespace c_hat
