#include "ArrayInitExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ArrayInitExpr::toString() const {
  std::string elementsStr = "";
  for (size_t i = 0; i < elements.size(); i++) {
    if (i > 0)
      elementsStr += ", ";
    elementsStr += elements[i]->toString();
  }
  return std::format("ArrayInitExpr([{}])", elementsStr);
}

} // namespace ast
} // namespace c_hat
