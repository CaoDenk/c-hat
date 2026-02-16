#pragma once

#include "Symbol.h"
#include "../types/Types.h"
#include <memory>

namespace c_hat {
namespace semantic {

class StructSymbol : public Symbol {
public:
  StructSymbol(const std::string &name,
               std::shared_ptr<types::Type> type,
               Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::Struct, visibility), type(type) {}

  std::shared_ptr<types::Type> getType() const { return type; }

private:
  std::shared_ptr<types::Type> type;
};

} // namespace semantic
} // namespace c_hat
