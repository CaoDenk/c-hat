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

  std::string name;
  std::vector<std::unique_ptr<Node>> arguments;
};

} // namespace ast
} // namespace c_hat
