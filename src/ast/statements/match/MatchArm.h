#pragma once

#include "../../Node.h"
#include "../../expressions/Expression.h"
#include "../Statement.h"
#include "Pattern.h"
#include <memory>


namespace c_hat {
namespace ast {

// 匹配分支
class MatchArm : public Node {
public:
  MatchArm(std::unique_ptr<Pattern> pattern, std::unique_ptr<Expression> guard,
           std::unique_ptr<Statement> body)
      : pattern(std::move(pattern)), guard(std::move(guard)),
        body(std::move(body)) {}

  NodeType getType() const override { return NodeType::MatchArm; }
  std::string toString() const override;

  std::unique_ptr<Pattern> pattern;
  std::unique_ptr<Expression> guard;
  std::unique_ptr<Statement> body;
};

} // namespace ast
} // namespace c_hat
