#pragma once

#include "Symbol.h"
#include "../types/Types.h"
#include <memory>

namespace c_hat {
namespace semantic {

// 变量符号
class VariableSymbol : public Symbol {
public:
  VariableSymbol(const std::string &name, std::shared_ptr<types::Type> type,
                 bool isMutable = false, bool isConst = false, Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::Variable, visibility), type(type), mutable_(isMutable), const_(isConst) {}

  // 获取变量类型
  std::shared_ptr<types::Type> getType() const { return type; }

  // 检查变量是否可变
  bool isMutable() const { return mutable_; }

  // 检查变量是否是编译期常量（constexpr）
  bool isConst() const { return const_; }

private:
  std::shared_ptr<types::Type> type;
  bool mutable_;
  bool const_;
};

} // namespace semantic
} // namespace c_hat
