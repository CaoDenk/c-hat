#pragma once

#include "../expressions/Expression.h"
#include "Statement.h"
#include <memory>

namespace c_hat {
namespace ast {

// 循环语句（含 foreach）
class ForStmt : public Statement {
public:
  // 普通 for 构造
  ForStmt(std::unique_ptr<Node> init, std::unique_ptr<Expression> condition,
          std::unique_ptr<Expression> update, std::unique_ptr<Statement> body)
      : init(std::move(init)), condition(std::move(condition)),
        update(std::move(update)), body(std::move(body)), isForeach(false) {}

  // foreach 构造: for (var x : collection)
  // init = 迭代变量声明, condition = 集合表达式
  ForStmt(std::unique_ptr<Node> iterVar,
          std::unique_ptr<Expression> collection,
          std::unique_ptr<Statement> body, bool)
      : init(std::move(iterVar)), condition(std::move(collection)),
        update(nullptr), body(std::move(body)), isForeach(true) {}

  // foreach 构造: for (var i, var x : collection)
  ForStmt(std::unique_ptr<Node> indexVar, std::unique_ptr<Node> iterVar,
          std::unique_ptr<Expression> collection,
          std::unique_ptr<Statement> body, bool)
      : init(std::move(iterVar)), condition(std::move(collection)),
        update(nullptr), body(std::move(body)), isForeach(true),
        indexVar(std::move(indexVar)) {}

  NodeType getType() const override { return NodeType::ForStmt; }
  std::string toString() const override;

  std::unique_ptr<Node> init;
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Expression> update;
  std::unique_ptr<Statement> body;
  bool isForeach = false; // true = range-for / foreach
  std::unique_ptr<Node> indexVar;
};

} // namespace ast
} // namespace c_hat
