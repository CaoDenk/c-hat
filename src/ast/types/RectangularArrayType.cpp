#include "RectangularArrayType.h"
#include <format>

namespace c_hat {
namespace ast {

std::string RectangularArrayType::toString() const {
  std::string result = std::format("RectangularArrayType({}, [", baseType->toString());
  for (size_t i = 0; i < sizes.size(); ++i) {
    if (i > 0) {
      result += ", ";
    }
    if (sizes[i]) {
      result += sizes[i]->toString();
    } else {
      result += "$";
    }
  }
  result += "])";
  return result;
}

} // namespace ast
} // namespace c_hat
