#include "UnaryExpr.h"
#include <format>

namespace c_hat {
namespace ast {

std::string UnaryExpr::toString() const {
  std::string opStr;
  switch (op) {
  case Op::Plus:
    opStr = "Plus";
    break;
  case Op::Minus:
    opStr = "Minus";
    break;
  case Op::Not:
    opStr = "Not";
    break;
  case Op::BitNot:
    opStr = "BitNot";
    break;
  case Op::AddressOf:
    opStr = "AddressOf";
    break;
  case Op::Dereference:
    opStr = "Dereference";
    break;
  case Op::Ref:
    opStr = "Ref";
    break;
  case Op::Await:
    opStr = "Await";
    break;
  case Op::Move:
    opStr = "Move";
    break;
  case Op::Immutable:
    opStr = "Immutable";
    break;
  case Op::At:
    opStr = "At";
    break;
  default:
    opStr = "Unknown";
    break;
  }
  return std::format("UnaryExpr({}, {})", opStr, expr->toString());
}

} // namespace ast
} // namespace c_hat
