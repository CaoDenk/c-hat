#pragma once

#include "../types/Types.h"
#include "Symbol.h"
#include <memory>

namespace c_hat {
namespace semantic {

// 函数符号
class FunctionSymbol : public Symbol {
public:
  FunctionSymbol(const std::string &name,
                 std::shared_ptr<types::FunctionType> type,
                 Visibility visibility = Visibility::Default,
                 bool isImmutable = false)
      : Symbol(name, SymbolType::Function, visibility), type(type), isImmutable(isImmutable) {}

  // 获取函数类型
  std::shared_ptr<types::FunctionType> getType() const { return type; }

  // 检查是否是不可变方法
  bool isImmutableMethod() const { return isImmutable; }

private:
  std::shared_ptr<types::FunctionType> type;
  bool isImmutable;
};

} // namespace semantic
} // namespace c_hat
