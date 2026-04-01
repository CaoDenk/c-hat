#pragma once

#include "../types/Types.h"
#include "Symbol.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace semantic {

class FunctionSymbol : public Symbol {
public:
  FunctionSymbol(const std::string &name,
                 std::shared_ptr<types::FunctionType> type,
                 Visibility visibility = Visibility::Default,
                 bool isImmutable = false, bool isVariadic = false)
      : Symbol(name, SymbolType::Function, visibility), type(type),
        isImmutable_(isImmutable), isExtern_(false), isVariadic_(isVariadic),
        isInherited_(false), isTemplate_(false) {}

  std::shared_ptr<types::FunctionType> getType() const { return type; }

  bool isImmutableMethod() const { return isImmutable_; }

  bool isExternFunction() const { return isExtern_; }

  bool isVariadicFunction() const { return isVariadic_; }

  bool isInherited() const { return isInherited_; }

  bool isTemplate() const { return isTemplate_; }

  const std::vector<std::string> &getTemplateParamNames() const {
    return templateParamNames_;
  }

  const std::string &getAbi() const { return abi_; }

  void setExtern(bool value) { this->isExtern_ = value; }

  void setAbi(const std::string &value) { this->abi_ = value; }

  void setVariadic(bool value) { this->isVariadic_ = value; }

  void setInherited(bool value) { this->isInherited_ = value; }

  void setTemplate(bool value) { this->isTemplate_ = value; }

  void setTemplateParamNames(const std::vector<std::string> &names) {
    this->templateParamNames_ = names;
  }

private:
  std::shared_ptr<types::FunctionType> type;
  bool isImmutable_;
  bool isExtern_;
  bool isVariadic_;
  bool isInherited_;
  bool isTemplate_;
  std::vector<std::string> templateParamNames_;
  std::string abi_;
};

} // namespace semantic
} // namespace c_hat
