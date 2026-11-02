#pragma once

#include "../Node.h"
#include "../others/AttributeApplication.h"
#include <vector>

namespace c_hat {
namespace ast {

class Declaration : public Node {
public:
  NodeType getType() const override { return NodeType::Declaration; }

  std::vector<std::unique_ptr<AttributeApplication>> attributes;
  // 导入模块的声明带模块前缀（如 A__），用于模块内非限定名解析。
  std::string modulePrefix;
};

} // namespace ast
} // namespace c_hat
