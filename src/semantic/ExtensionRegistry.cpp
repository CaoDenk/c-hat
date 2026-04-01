#include "ExtensionRegistry.h"
#include "../ast/declarations/FunctionDecl.h"
#include "../ast/declarations/GetterDecl.h"
#include "../ast/declarations/SetterDecl.h"
#include "../ast/declarations/VariableDecl.h"
#include "../types/ClassType.h"
#include "../types/PrimitiveType.h"

namespace c_hat {
namespace semantic {

void ExtensionRegistry::addExtension(std::shared_ptr<types::Type> extendedType,
                                     ast::ExtensionDecl *extension) {
  std::string key = getTypeKey(extendedType);
  extensions_[key].push_back(extension);
}

std::vector<ast::ExtensionDecl *> ExtensionRegistry::getExtensionsForType(
    std::shared_ptr<types::Type> type) const {
  std::string key = getTypeKey(type);
  auto it = extensions_.find(key);
  if (it != extensions_.end()) {
    return it->second;
  }
  return {};
}

ast::Declaration *
ExtensionRegistry::findStaticMember(std::shared_ptr<types::Type> type,
                                    const std::string &name) const {
  auto exts = getExtensionsForType(type);
  for (auto *ext : exts) {
    for (const auto &member : ext->members) {
      if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(member.get())) {
        if (funcDecl->name == name && funcDecl->isStatic) {
          return funcDecl;
        }
      }
      if (auto *varDecl = dynamic_cast<ast::VariableDecl *>(member.get())) {
        if (varDecl->name == name &&
            varDecl->specifiers.find("static") != std::string::npos) {
          return varDecl;
        }
      }
      if (auto *getterDecl = dynamic_cast<ast::GetterDecl *>(member.get())) {
        if (getterDecl->name == name &&
            getterDecl->specifiers.find("static") != std::string::npos) {
          return getterDecl;
        }
      }
    }
  }
  return nullptr;
}

ast::Declaration *
ExtensionRegistry::findInstanceMember(std::shared_ptr<types::Type> type,
                                      const std::string &name) const {
  auto exts = getExtensionsForType(type);
  for (auto *ext : exts) {
    for (const auto &member : ext->members) {
      if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(member.get())) {
        if (funcDecl->name == name && !funcDecl->isStatic) {
          return funcDecl;
        }
      }
      if (auto *getterDecl = dynamic_cast<ast::GetterDecl *>(member.get())) {
        if (getterDecl->name == name &&
            getterDecl->specifiers.find("static") == std::string::npos) {
          return getterDecl;
        }
      }
      if (auto *setterDecl = dynamic_cast<ast::SetterDecl *>(member.get())) {
        if (setterDecl->name == name) {
          return setterDecl;
        }
      }
    }
  }
  return nullptr;
}

void ExtensionRegistry::clear() { extensions_.clear(); }

std::string
ExtensionRegistry::getTypeKey(std::shared_ptr<types::Type> type) const {
  if (!type) {
    return "";
  }

  if (type->isClass()) {
    auto classType = std::dynamic_pointer_cast<types::ClassType>(type);
    return "class:" + classType->getName();
  }

  if (type->isPrimitive()) {
    auto primType = std::dynamic_pointer_cast<types::PrimitiveType>(type);
    return "primitive:" + primType->toString();
  }

  return type->toString();
}

} // namespace semantic
} // namespace c_hat
