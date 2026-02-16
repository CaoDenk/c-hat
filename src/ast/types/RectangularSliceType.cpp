#include "RectangularSliceType.h"
#include <format>

namespace c_hat {
namespace ast {

std::string RectangularSliceType::toString() const {
  std::string result = std::format("RectangularSliceType({}, {})", baseType->toString(), rank);
  return result;
}

} // namespace ast
} // namespace c_hat
