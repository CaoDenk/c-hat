#pragma once

#include "Symbol.h"
#include "../types/Types.h"
#include <memory>

namespace c_hat {
namespace semantic {

// 类符号
class ClassSymbol : public Symbol {
public:
  ClassSymbol(const std::string &name, std::shared_ptr<types::Type> type,
              Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::Class, visibility), type(type) {}

  // 获取类类型
  std::shared_ptr<types::Type> getType() const { return type; }

private:
  std::shared_ptr<types::Type> type;
};

} // namespace semantic
} // namespace c_hat
