#pragma once

#include "Type.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

// 泛型类型
class GenericType : public Type {
public:
  GenericType(const std::string &name,
              std::vector<std::unique_ptr<Node>> arguments)
      : name(name), arguments(std::move(arguments)) {}

  NodeType getType() const override { return NodeType::GenericType; }
  std::string toString() const override;
  // 暂时不克隆泛型参数，避免 Node 没有 clone() 的问题
  std::unique_ptr<Type> clone() const override { return nullptr; }

  std::string name;
  std::vector<std::unique_ptr<Node>> arguments;
};

} // namespace ast
} // namespace c_hat
