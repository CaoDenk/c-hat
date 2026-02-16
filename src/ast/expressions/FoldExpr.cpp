#include "FoldExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string FoldExpr::toString() const {
  std::string foldTypeStr;
  switch (foldType) {
  case FoldType::Right:
    foldTypeStr = "Right";
    break;
  case FoldType::Left:
    foldTypeStr = "Left";
    break;
  case FoldType::Binary:
    foldTypeStr = "Binary";
    break;
  default:
    foldTypeStr = "Unknown";
    break;
  }
  if (right) {
    return std::format("FoldExpr({}, {}, {}, {})", foldTypeStr,
                       expr->toString(), op, right->toString());
  }
  return std::format("FoldExpr({}, {}, {})", foldTypeStr, expr->toString(), op);
}

} // namespace ast
} // namespace c_hat
