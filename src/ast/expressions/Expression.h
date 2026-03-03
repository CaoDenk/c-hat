#pragma once

#include "../Node.h"
#include <memory>

namespace c_hat {
namespace ast {

// 表达式节点
class Expression : public Node {
public:
  NodeType getType() const override { return NodeType::Expression; }
  virtual std::unique_ptr<Expression> clone() const = 0;
};

} // namespace ast
} // namespace c_hat
