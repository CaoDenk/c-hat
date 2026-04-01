#pragma once

#include "../ast/declarations/ExtensionDecl.h"
#include "../types/Type.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace c_hat {
namespace semantic {

class ExtensionRegistry {
public:
  void addExtension(std::shared_ptr<types::Type> extendedType,
                    ast::ExtensionDecl *extension);

  std::vector<ast::ExtensionDecl *>
  getExtensionsForType(std::shared_ptr<types::Type> type) const;

  ast::Declaration *findStaticMember(std::shared_ptr<types::Type> type,
                                     const std::string &name) const;

  ast::Declaration *findInstanceMember(std::shared_ptr<types::Type> type,
                                       const std::string &name) const;

  void clear();

private:
  std::unordered_map<std::string, std::vector<ast::ExtensionDecl *>> extensions_;

  std::string getTypeKey(std::shared_ptr<types::Type> type) const;
};

} // namespace semantic
} // namespace c_hat
