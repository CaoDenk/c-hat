#pragma once

#include "../../others/Parameter.h"
#include "../Statement.h"
#include <memory>


namespace c_hat {
namespace ast {

// catch语句
class CatchStmt : public Statement {
public:
  CatchStmt(std::unique_ptr<Parameter> param, std::unique_ptr<Statement> body)
      : param(std::move(param)), body(std::move(body)) {}

  NodeType getType() const override { return NodeType::CatchStmt; }
  std::string toString() const override;

  std::unique_ptr<Parameter> param;
  std::unique_ptr<Statement> body;
  // 语义层填写的 catch 类型标签；0 表示 catch(...)。
  int catchTag = 0;
  // 类/接口 catch 的静态类型信息，用于基类/接口匹配展开。
  std::string exceptionTypeName;
  bool exceptionTypeIsPointer = false;
  bool exceptionTypeIsInterface = false;
};

} // namespace ast
} // namespace c_hat
