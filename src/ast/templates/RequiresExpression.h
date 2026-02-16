#pragma once

#include "../Node.h"
#include "Requirement.h"
#include <memory>
#include <vector>

namespace c_hat {
namespace ast {

// requires表达式
class RequiresExpression : public Node {
public:
  RequiresExpression(std::vector<std::unique_ptr<Node>> params,
                     std::vector<std::unique_ptr<Requirement>> requirements)
      : params(std::move(params)), requirements(std::move(requirements)) {}

  NodeType getType() const override { return NodeType::RequiresExpression; }
  std::string toString() const override;

  std::vector<std::unique_ptr<Node>> params;              // 参数列表
  std::vector<std::unique_ptr<Requirement>> requirements; // 需求列表
};

} // namespace ast
} // namespace c_hat
