
#include "ModuleLoader.h"
#include "../parser/Parser.h"
#include <fstream>
#include <print>
#include <stdexcept>


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
  fs::path basePath;
  if (!stdlibPath_.empty()) {
    basePath = stdlibPath_;
  } else {
    basePath = "stdlib";
  }

  for (const auto &part : modulePath) {
    basePath /= part;
  }

  auto filePath1 = basePath;
  filePath1 += ".ch";

  auto filePath2 = basePath / "mod.ch";

  if (fs::exists(filePath1)) {
    return filePath1;
  }
  if (fs::exists(filePath2)) {
    return filePath2;
  }

  return filePath1;
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

} // namespace semantic
} // namespace c_hat
