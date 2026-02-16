#pragma once

#include "NodeType.h"
#include <string>

namespace c_hat {
namespace ast {

// 基础节点类
class Node {
public:
  virtual ~Node() = default;

  virtual NodeType getType() const = 0;
  virtual std::string toString() const = 0;
};

} // namespace ast
} // namespace c_hat
