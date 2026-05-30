#pragma once

#include "Symbol.h"
#include <string>

namespace c_hat {
namespace semantic {

class ConceptSymbol : public Symbol {
public:
  ConceptSymbol(const std::string &name,
                Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::Concept, visibility) {}
};

} // namespace semantic
} // namespace c_hat
