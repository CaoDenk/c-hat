
#pragma once

#include "../ast/AstNodes.h"
#include "ModuleSymbol.h"
#include "SymbolTable.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace c_hat {
namespace semantic {

namespace fs = std::filesystem;

struct ModuleFunctionAlias {
  std::string name;
  std::string linkName;
};

class ModuleLoader {
public:
  ModuleLoader(const std::string &stdlibPath) : stdlibPath_(stdlibPath) {
    if (!stdlibPath.empty()) {
      modulePaths_.push_back(stdlibPath);
    }
  }

  ModuleLoader(const std::vector<std::string> &modulePaths)
      : modulePaths_(modulePaths) {
    if (!modulePaths.empty()) {
      stdlibPath_ = modulePaths[0];
    }
  }

  void addModulePath(const std::string &path) {
    modulePaths_.push_back(path);
  }

  std::unique_ptr<ast::Program>
  loadModule(const std::vector<std::string> &modulePath);

  bool isModuleLoaded(const std::vector<std::string> &modulePath) const;

  const std::unordered_set<std::string> &getLoadedModules() const {
    return loadedModules_;
  }

  std::string
  modulePathToString(const std::vector<std::string> &modulePath) const;

  void addFunctionAlias(const std::string &name, const std::string &linkName);

  const std::vector<ModuleFunctionAlias> &getFunctionAliases() const {
    return functionAliases_;
  }

private:
  std::string stdlibPath_;
  std::vector<std::string> modulePaths_;
  std::unordered_set<std::string> loadedModules_;
  std::unordered_set<std::string> loadingModules_;
  std::vector<ModuleFunctionAlias> functionAliases_;

  fs::path modulePathToFilePath(const std::vector<std::string> &modulePath);

  std::unique_ptr<ast::Program> parseFile(const fs::path &filePath);
};

// 递归加载 import 的模块，并把模块声明追加到主程序。
void loadImportsInto(ast::Program &program, ModuleLoader &loader);

} // namespace semantic
} // namespace c_hat
