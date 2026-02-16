#include "BinaryExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string BinaryExpr::toString() const {
  std::string opStr;
  switch (op) {
  case Op::Add:
    opStr = "Add";
    break;
  case Op::Sub:
    opStr = "Sub";
    break;
  case Op::Mul:
    opStr = "Mul";
    break;
  case Op::Div:
    opStr = "Div";
    break;
  default:
    opStr = "Unknown";
    break;
  }
  return std::format("BinaryExpr({}, {}, {})", opStr, left->toString(),
                     right->toString());
}

} // namespace ast
} // namespace c_hat
