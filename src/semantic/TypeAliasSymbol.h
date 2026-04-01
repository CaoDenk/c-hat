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
                  Visibility visibility = Visibility::Default, bool isTypeSet = false)
      : Symbol(name, SymbolType::TypeAlias, visibility), type(type), isTypeSet(isTypeSet) {}

  // 获取类型别名指向的类型
  std::shared_ptr<types::Type> getType() const { return type; }
  
  // 是否是类型集合别名 (using X = A | B | C)
  bool isTypeSetAlias() const { return isTypeSet; }

private:
  std::shared_ptr<types::Type> type;
  bool isTypeSet;
};

} // namespace semantic
} // namespace c_hat
