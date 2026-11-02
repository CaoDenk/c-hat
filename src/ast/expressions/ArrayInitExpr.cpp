#include "ArrayInitExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ArrayInitExpr::toString() const {
  if (isRepeat()) {
    return std::format("ArrayInitExpr([{}; {}])", repeatValue->toString(),
                       repeatCount->toString());
  }
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
