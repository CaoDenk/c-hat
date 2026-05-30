#pragma once

#include "Expression.h"
#include <memory>

namespace c_hat {
namespace ast {

// 编译期字段访问表达式: obj.[field]
class MetaFieldAccessExpr : public Expression {
public:
  MetaFieldAccessExpr(std::unique_ptr<Expression> object,
                      std::unique_ptr<Expression> field)
      : object(std::move(object)), field(std::move(field)) {}

  NodeType getType() const override { return NodeType::MetaFieldAccessExpr; }

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<MetaFieldAccessExpr>(object->clone(),
                                                 field->clone());
  }

  std::string toString() const override {
    return object->toString() + ".[" + field->toString() + "]";
  }

  std::unique_ptr<Expression> object;
  std::unique_ptr<Expression> field;
};

} // namespace ast
} // namespace c_hat
