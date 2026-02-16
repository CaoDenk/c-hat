#include "StructInitExpr.h"

namespace c_hat {
namespace ast {

std::string StructInitExpr::toString() const {
  std::string result = "StructInitExpr{";
  for (size_t i = 0; i < fields.size(); ++i) {
    result += fields[i].first + ": " + fields[i].second->toString();
    if (i < fields.size() - 1) {
      result += ", ";
    }
  }
  result += "}";
  return result;
}

} // namespace ast
} // namespace c_hat
