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
                 Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::Function, visibility), type(type) {}

  // 获取函数类型
  std::shared_ptr<types::FunctionType> getType() const { return type; }

private:
  std::shared_ptr<types::FunctionType> type;
};

} // namespace semantic
} // namespace c_hat
