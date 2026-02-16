#pragma once

#include "Symbol.h"
#include <string>
#include <vector>

namespace c_hat {
namespace semantic {

class ModuleSymbol : public Symbol {
public:
  ModuleSymbol(const std::string &name,
               Visibility visibility = Visibility::Default)
      : Symbol(name, SymbolType::Module, visibility) {}

  std::string getFullPath() const {
    std::string path;
    for (size_t i = 0; i < modulePath.size(); ++i) {
      if (i > 0)
        path += ".";
      path += modulePath[i];
    }
    return path;
  }

  std::vector<std::string> modulePath;
};

} // namespace semantic
} // namespace c_hat
