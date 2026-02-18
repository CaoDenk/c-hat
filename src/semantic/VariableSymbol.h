#pragma once

#include "Symbol.h"
#include "../ast/declarations/VariableDecl.h"
#include "../types/Types.h"
#include <memory>

namespace c_hat {
namespace semantic {

// 变量符号
class VariableSymbol : public Symbol {
public:
  VariableSymbol(const std::string &name, std::shared_ptr<types::Type> type,
                 ast::VariableKind kind = ast::VariableKind::Explicit,
                 bool isConst = false, Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::Variable, visibility), type(type), kind_(kind), const_(isConst) {}

  // 获取变量类型
  std::shared_ptr<types::Type> getType() const { return type; }

  // 获取变量声明的 kind
  ast::VariableKind getKind() const { return kind_; }

  // 检查变量是否是 let 声明的（不可重新赋值）
  bool isImmutableBinding() const { return kind_ == ast::VariableKind::Let; }

  // 检查变量是否是编译期常量（constexpr）
  bool isConst() const { return const_; }

private:
  std::shared_ptr<types::Type> type;
  ast::VariableKind kind_;
  bool const_;
};

} // namespace semantic
} // namespace c_hat
