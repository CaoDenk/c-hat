#include "FunctionType.h"
#include <sstream>

namespace c_hat {
namespace ast {

std::string FunctionType::toString() const {
  std::ostringstream oss;
  oss << "func(";
  for (size_t i = 0; i < parameterTypes.size(); ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << parameterTypes[i]->toString();
  }
  oss << ") -> " << returnType->toString();
  return oss.str();
}

} // namespace ast
} // namespace c_hat
