#pragma once

#include "Expression.h"
#include "../types/Type.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// sizeof(Type)：编译期求值，返回 long。
class SizeofExpr : public Expression {
public:
  explicit SizeofExpr(std::unique_ptr<Type> type) : type(std::move(type)) {}

  NodeType getType() const override { return NodeType::SizeofExpr; }
  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<SizeofExpr>(type->clone());
  }
  std::string toString() const override {
    return "sizeof(" + type->toString() + ")";
  }

  std::unique_ptr<Type> type;
};

} // namespace ast
} // namespace c_hat
