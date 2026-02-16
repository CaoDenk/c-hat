#pragma once

#include "../Node.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

// 模板参数
class TemplateParameter : public Node {
public:
  TemplateParameter(const std::string &name,
                    std::unique_ptr<Node> constraint = nullptr,
                    bool isVariadic = false)
      : name(name), constraint(std::move(constraint)), isVariadic(isVariadic) {}

  NodeType getType() const override { return NodeType::TemplateParameter; }
  std::string toString() const override {
    return "TemplateParameter(" + name + ")";
  }

  std::string name;
  std::unique_ptr<Node> constraint;
  bool isVariadic;
};

} // namespace ast
} // namespace c_hat
