#pragma once

#include "../others/Parameter.h"
#include "../statements/Statement.h"
#include "Expression.h"
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

struct Capture {
  std::string name;
  bool byRef;
  bool isMove;

  Capture(const std::string &name, bool byRef = false, bool isMove = false)
      : name(name), byRef(byRef), isMove(isMove) {}
};

class LambdaExpr : public Expression {
public:
  LambdaExpr(std::vector<std::unique_ptr<Parameter>> params,
             std::unique_ptr<Statement> body,
             std::vector<Capture> captures = {})
      : params(std::move(params)), body(std::move(body)),
        captures(std::move(captures)) {}

  NodeType getType() const override { return NodeType::LambdaExpr; }
  std::string toString() const override;
  std::unique_ptr<Expression> clone() const override {
    // 注意：这里我们暂时无法克隆 Parameter 和 Statement 类型，因为它们没有 clone 方法
    // 这是一个临时解决方案，实际应该为 Parameter 和 Statement 类也添加 clone 方法
    return std::make_unique<LambdaExpr>(std::vector<std::unique_ptr<Parameter>>(), nullptr, captures);
  }

  std::vector<std::unique_ptr<Parameter>> params;
  std::unique_ptr<Statement> body;
  std::vector<Capture> captures;
};

} // namespace ast
} // namespace c_hat
