#pragma once

#include "../Node.h"
#include "../expressions/Expression.h"
#include "../types/Type.h"
#include <memory>
#include <string>
#include <utility>

namespace c_hat {
namespace ast {

// 参数
class Parameter : public Node {
public:
  Parameter(const std::string &name, std::unique_ptr<Type> type,
            std::unique_ptr<Expression> defaultValue = nullptr)
      : name(name), type(std::move(type)),
        defaultValue(std::move(defaultValue)) {}

  NodeType getType() const override { return NodeType::Parameter; }
  std::string toString() const override;

  std::string name;
  std::unique_ptr<Type> type;
  std::unique_ptr<Expression> defaultValue;
};

} // namespace ast
} // namespace c_hat
