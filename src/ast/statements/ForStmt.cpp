#include "ForStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ForStmt::toString() const {
  return std::format("ForStmt({}, {}, {}, {})", init->toString(),
                     condition->toString(), update->toString(),
                     body->toString());
}

} // namespace ast
} // namespace c_hat
