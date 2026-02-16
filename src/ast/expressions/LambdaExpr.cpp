#include "LambdaExpr.h"
#include "../statements/ExprStmt.h"
#include <format>

namespace c_hat {
namespace ast {

std::string LambdaExpr::toString() const {
  std::string result = "[";

  if (!captures.empty()) {
    for (size_t i = 0; i < captures.size(); ++i) {
      if (i > 0) {
        result += ", ";
      }
      if (captures[i].byRef) {
        result += "&";
      }
      if (captures[i].isMove) {
        result += "= ";
      }
      result += captures[i].name;
    }
  }

  result += "](";

  for (size_t i = 0; i < params.size(); ++i) {
    if (i > 0) {
      result += ", ";
    }
    result += params[i]->name;
  }

  result += ") ";

  if (auto *exprStmt = dynamic_cast<ExprStmt *>(body.get())) {
    result += "=> ";
    result += exprStmt->expr->toString();
  } else {
    result += body->toString();
  }

  return result;
}

} // namespace ast
} // namespace c_hat
