#pragma once

#include "../Node.h"
#include <memory>

namespace c_hat {
namespace ast {

// 类型节点
class Type : public Node {
public:
  NodeType getType() const override { return NodeType::Type; }
  virtual std::unique_ptr<Type> clone() const { return nullptr; }
};

} // namespace ast
} // namespace c_hat
