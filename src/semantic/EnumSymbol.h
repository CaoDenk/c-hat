#pragma once

#include "../types/Types.h"
#include "Symbol.h"
#include <memory>


namespace c_hat {
namespace semantic {

class EnumSymbol : public Symbol {
public:
  EnumSymbol(const std::string &name, std::shared_ptr<types::Type> type,
             Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::Enum, visibility), type(type) {}

  std::shared_ptr<types::Type> getType() const { return type; }

private:
  std::shared_ptr<types::Type> type;
};

} // namespace semantic
} // namespace c_hat
