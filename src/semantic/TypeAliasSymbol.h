#pragma once

#include "../types/Types.h"
#include "Symbol.h"
#include <memory>

namespace c_hat {
namespace semantic {

// 类型别名符号
class TypeAliasSymbol : public Symbol {
public:
  TypeAliasSymbol(const std::string &name, std::shared_ptr<types::Type> type,
                  Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::TypeAlias, visibility), type(type) {}

  // 获取类型别名指向的类型
  std::shared_ptr<types::Type> getType() const { return type; }

private:
  std::shared_ptr<types::Type> type;
};

} // namespace semantic
} // namespace c_hat
