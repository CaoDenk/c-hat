
#include "ModuleLoader.h"
#include "../parser/Parser.h"
#include <fstream>
#include <print>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>


namespace c_hat {
namespace semantic {

std::string ModuleLoader::modulePathToString(
    const std::vector<std::string> &modulePath) const {
  std::string result;
  for (size_t i = 0; i < modulePath.size(); ++i) {
    if (i > 0) {
      result += ".";
    }
    result += modulePath[i];
  }
  return result;
}

fs::path
ModuleLoader::modulePathToFilePath(const std::vector<std::string> &modulePath) {
  std::vector<fs::path> searchPaths;

  if (!modulePaths_.empty()) {
    for (const auto &p : modulePaths_) {
      searchPaths.push_back(fs::path(p));
    }
  } else {
    searchPaths.push_back(fs::path("stdlib"));
  }

  for (const auto &basePath : searchPaths) {
    fs::path moduleBase = basePath;
    for (const auto &part : modulePath) {
      moduleBase /= part;
    }

    auto filePath1 = moduleBase;
    filePath1 += ".ch";

    auto filePath2 = moduleBase / "mod.ch";

    if (fs::exists(filePath1)) {
      return filePath1;
    }
    if (fs::exists(filePath2)) {
      return filePath2;
    }
  }

  fs::path defaultPath;
  if (!modulePaths_.empty()) {
    defaultPath = fs::path(modulePaths_[0]);
  } else {
    defaultPath = fs::path("stdlib");
  }
  for (const auto &part : modulePath) {
    defaultPath /= part;
  }
  defaultPath += ".ch";

  return defaultPath;
}

std::unique_ptr<ast::Program>
ModuleLoader::parseFile(const fs::path &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open module file: " +
                             filePath.string());
  }

  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
  file.close();

  c_hat::parser::Parser parser(source);
  auto program = parser.parseProgram();

  return program;
}

std::unique_ptr<ast::Program>
ModuleLoader::loadModule(const std::vector<std::string> &modulePath) {
  std::string moduleName = modulePathToString(modulePath);

  if (loadedModules_.find(moduleName) != loadedModules_.end()) {
    return nullptr;
  }

  if (loadingModules_.find(moduleName) != loadingModules_.end()) {
    throw std::runtime_error("Circular dependency detected in module: " +
                             moduleName);
  }

  loadingModules_.insert(moduleName);

  auto filePath = modulePathToFilePath(modulePath);
  auto program = parseFile(filePath);

  loadingModules_.erase(moduleName);
  loadedModules_.insert(moduleName);

  return program;
}

bool ModuleLoader::isModuleLoaded(
    const std::vector<std::string> &modulePath) const {
  std::string moduleName = modulePathToString(modulePath);
  return loadedModules_.find(moduleName) != loadedModules_.end();
}


namespace {

std::string topLevelName(ast::Declaration *decl) {
  if (auto *func = dynamic_cast<ast::FunctionDecl *>(decl)) {
    return func->name;
  }
  if (auto *cls = dynamic_cast<ast::ClassDecl *>(decl)) {
    return cls->name;
  }
  if (auto *st = dynamic_cast<ast::StructDecl *>(decl)) {
    return st->name;
  }
  if (auto *var = dynamic_cast<ast::VariableDecl *>(decl)) {
    return var->name;
  }
  return {};
}

std::string moduleFunctionPrefix(
    const std::vector<std::string> &modulePath) {
  std::string prefix;
  for (size_t i = 0; i < modulePath.size(); ++i) {
    if (i > 0) {
      prefix += "__";
    }
    prefix += modulePath[i];
  }
  if (!prefix.empty()) {
    prefix += "__";
  }
  return prefix;
}

void appendLoadedProgram(
    ast::Program &target, std::unique_ptr<ast::Program> loaded,
    ModuleLoader &loader, const std::vector<std::string> &modulePath,
    std::unordered_set<std::string> &seen,
    std::unordered_map<std::string, std::vector<std::string>> &candidates,
    std::unordered_map<std::string, std::vector<std::string>> &typeCandidates,
    std::unordered_map<std::string, std::vector<std::string>> &varCandidates) {
  if (!loaded) {
    return;
  }

  // 先递归加载被导入模块自身的 import。
  std::vector<std::pair<std::vector<std::string>,
                        std::unique_ptr<ast::Program>>>
      nested;
  for (auto &decl : loaded->declarations) {
    if (auto *importDecl = dynamic_cast<ast::ImportDecl *>(decl.get())) {
      nested.emplace_back(importDecl->modulePath,
                          loader.loadModule(importDecl->modulePath));
    }
  }
  for (auto &[childPath, child] : nested) {
    appendLoadedProgram(target, std::move(child), loader, childPath, seen,
                        candidates, typeCandidates, varCandidates);
  }

  const std::string prefix = moduleFunctionPrefix(modulePath);
  // 模块自由函数加模块前缀；类/结构体成员函数不改名。
  for (auto &decl : loaded->declarations) {
    if (auto *func = dynamic_cast<ast::FunctionDecl *>(decl.get())) {
      if (func->name != "main" && !prefix.empty()) {
        std::string originalName = func->name;
        func->name = prefix + originalName;
        candidates[originalName].push_back(func->name);
      }
    }
  }

  // 模块类型加模块前缀；同时修正模块内构造函数/析构函数名。
  for (auto &decl : loaded->declarations) {
    std::string oldName;
    std::vector<std::unique_ptr<ast::Node>> *members = nullptr;
    if (auto *cls = dynamic_cast<ast::ClassDecl *>(decl.get())) {
      oldName = cls->name;
      members = &cls->members;
    } else if (auto *st = dynamic_cast<ast::StructDecl *>(decl.get())) {
      oldName = st->name;
      members = &st->members;
    }
    if (!oldName.empty() && !prefix.empty()) {
      std::string newName = prefix + oldName;
      if (auto *cls = dynamic_cast<ast::ClassDecl *>(decl.get())) {
        cls->name = newName;
      } else if (auto *st = dynamic_cast<ast::StructDecl *>(decl.get())) {
        st->name = newName;
      }
      typeCandidates[oldName].push_back(newName);
      for (auto &member : *members) {
        if (auto *func = dynamic_cast<ast::FunctionDecl *>(member.get())) {
          if (func->name == oldName) {
            func->name = newName;
          } else if (func->name == "~" + oldName) {
            func->name = "~" + newName;
          }
        }
      }
    }
  }

  // 模块顶层变量加模块前缀。
  for (auto &decl : loaded->declarations) {
    if (auto *var = dynamic_cast<ast::VariableDecl *>(decl.get())) {
      if (!prefix.empty()) {
        std::string originalName = var->name;
        var->name = prefix + originalName;
        varCandidates[originalName].push_back(var->name);
      }
    }
  }

  // import/module 声明不进入 codegen；其余声明追加到主程序。
  for (auto &decl : loaded->declarations) {
    if (dynamic_cast<ast::ImportDecl *>(decl.get()) ||
        dynamic_cast<ast::ModuleDecl *>(decl.get())) {
      continue;
    }
    // 自由函数已按模块前缀隔离；类/结构体/顶层变量仍检查冲突。
    if (!dynamic_cast<ast::FunctionDecl *>(decl.get())) {
      std::string name = topLevelName(decl.get());
      if (!name.empty() && !seen.insert(name).second) {
        throw std::runtime_error(
            "Duplicate top-level symbol across modules: " + name);
      }
    }
    target.declarations.push_back(std::move(decl));
  }
}

} // namespace

void ModuleLoader::addFunctionAlias(const std::string &name,
                                    const std::string &linkName) {
  if (name.empty() || linkName.empty() || name == linkName) {
    return;
  }
  functionAliases_.push_back({name, linkName});
}

void ModuleLoader::addVariableAlias(const std::string &name,
                                   const std::string &linkName) {
  if (name.empty() || linkName.empty() || name == linkName) {
    return;
  }
  variableAliases_.push_back({name, linkName});
}

void ModuleLoader::addTypeAlias(const std::string &name,
                               const std::string &linkName) {
  if (name.empty() || linkName.empty() || name == linkName) {
    return;
  }
  typeAliases_.push_back({name, linkName});
}

void loadImportsInto(ast::Program &program, ModuleLoader &loader) {
  std::unordered_set<std::string> seen;
  std::unordered_set<std::string> mainNames;
  for (auto &decl : program.declarations) {
    std::string name = topLevelName(decl.get());
    if (!name.empty()) {
      seen.insert(name);
      mainNames.insert(name);
    }
  }

  std::vector<std::pair<std::vector<std::string>,
                        std::unique_ptr<ast::Program>>>
      imported;
  for (auto &decl : program.declarations) {
    if (auto *importDecl = dynamic_cast<ast::ImportDecl *>(decl.get())) {
      imported.emplace_back(importDecl->modulePath,
                            loader.loadModule(importDecl->modulePath));
    }
  }

  std::unordered_map<std::string, std::vector<std::string>> candidates;
  std::unordered_map<std::string, std::vector<std::string>> typeCandidates;
  std::unordered_map<std::string, std::vector<std::string>> varCandidates;
  const size_t mainDeclarationCount = program.declarations.size();
  for (auto &[path, moduleProgram] : imported) {
    appendLoadedProgram(program, std::move(moduleProgram), loader, path, seen,
                        candidates, typeCandidates, varCandidates);
  }

  // 唯一且不与主程序冲突的短名生成别名，支持 import 后非限定调用。
  for (const auto &[name, links] : candidates) {
    if (links.size() == 1 && !mainNames.count(name)) {
      loader.addFunctionAlias(name, links.front());
    }
  }
  for (const auto &[name, links] : typeCandidates) {
    if (links.size() == 1 && !mainNames.count(name)) {
      loader.addTypeAlias(name, links.front());
    }
  }
  for (const auto &[name, links] : varCandidates) {
    if (links.size() == 1 && !mainNames.count(name)) {
      loader.addVariableAlias(name, links.front());
    }
  }

  // 排序：主程序 import 声明 -> 导入模块声明 -> 主程序其余声明。
  // 这样模块符号先注册，导入函数/类也先于主程序函数体分析。
  if (mainDeclarationCount <= program.declarations.size()) {
    std::vector<std::unique_ptr<ast::Declaration>> mainDecls;
    std::vector<std::unique_ptr<ast::Declaration>> importedDecls;
    for (size_t i = 0; i < program.declarations.size(); ++i) {
      if (i < mainDeclarationCount) {
        mainDecls.push_back(std::move(program.declarations[i]));
      } else {
        importedDecls.push_back(std::move(program.declarations[i]));
      }
    }
    std::vector<std::unique_ptr<ast::Declaration>> ordered;
    std::vector<std::unique_ptr<ast::Declaration>> remaining;
    for (auto &decl : mainDecls) {
      if (dynamic_cast<ast::ImportDecl *>(decl.get())) {
        ordered.push_back(std::move(decl));
      } else {
        remaining.push_back(std::move(decl));
      }
    }
    for (auto &decl : importedDecls) {
      ordered.push_back(std::move(decl));
    }
    for (auto &decl : remaining) {
      ordered.push_back(std::move(decl));
    }
    program.declarations = std::move(ordered);
  }
}

} // namespace semantic
} // namespace c_hat
