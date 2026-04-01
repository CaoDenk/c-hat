#include "SymbolTable.h"

namespace c_hat {
namespace semantic {

// SymbolTable 构造函数
SymbolTable::SymbolTable() : currentScopeLevel(0) {
  // 初始化全局作用域
  scopes.emplace_back();
}

// 进入一个新的作用域
void SymbolTable::enterScope() {
  currentScopeLevel++;
  scopes.emplace_back();
}

// 退出当前作用域
void SymbolTable::exitScope() {
  if (currentScopeLevel > 0) {
    currentScopeLevel--;
    scopes.pop_back();
  }
}

// 添加符号
void SymbolTable::addSymbol(std::shared_ptr<Symbol> symbol) {
  // 设置符号所在作用域
  symbol->setScopeLevel(currentScopeLevel);

  // 将符号添加到当前作用域
  scopes.back()[symbol->getName()].push_back(symbol);
}

// 查找符号（从当前作用域开始向上查找）
std::shared_ptr<Symbol> SymbolTable::lookupSymbol(const std::string &name) {
  // 从当前作用域开始向上查找
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    const auto &scope = *it;
    auto symbolIt = scope.find(name);
    if (symbolIt != scope.end() && !symbolIt->second.empty()) {
      // 对于非函数符号，返回第一个找到的
      // 对于函数符号，这里只返回第一个，函数重载使用lookupFunctionSymbols
      return symbolIt->second[0];
    }
  }

  // 未找到符号
  return nullptr;
}

// 查找所有同名函数符号（从当前作用域开始向上查找）
std::vector<std::shared_ptr<FunctionSymbol>>
SymbolTable::lookupFunctionSymbols(const std::string &name) {
  std::vector<std::shared_ptr<FunctionSymbol>> result;

  // 从当前作用域开始向上查找
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    const auto &scope = *it;
    auto symbolIt = scope.find(name);
    if (symbolIt != scope.end()) {
      for (const auto &symbol : symbolIt->second) {
        if (symbol->getType() == SymbolType::Function) {
          auto funcSymbol = std::dynamic_pointer_cast<FunctionSymbol>(symbol);
          if (funcSymbol) {
            result.push_back(funcSymbol);
          }
        }
      }
    }
  }

  return result;
}

// 检查当前作用域是否已存在该符号
bool SymbolTable::hasSymbolInCurrentScope(const std::string &name) const {
  if (scopes.empty()) {
    return false;
  }

  const auto &currentScope = scopes.back();
  auto symbolIt = currentScope.find(name);
  return symbolIt != currentScope.end() && !symbolIt->second.empty();
}

// 移除符号（用于方法重写时移除继承的方法）
void SymbolTable::removeSymbol(const std::string &name,
                                std::shared_ptr<Symbol> symbol) {
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    auto &scope = *it;
    auto symbolIt = scope.find(name);
    if (symbolIt != scope.end()) {
      auto &symbols = symbolIt->second;
      for (auto vecIt = symbols.begin(); vecIt != symbols.end(); ++vecIt) {
        if (*vecIt == symbol) {
          symbols.erase(vecIt);
          if (symbols.empty()) {
            scope.erase(symbolIt);
          }
          return;
        }
      }
    }
  }
}

// 获取上一级作用域中的所有符号
std::vector<std::shared_ptr<Symbol>> SymbolTable::getSymbolsInParentScope() const {
  std::vector<std::shared_ptr<Symbol>> result;

  // 检查是否有上一级作用域
  if (scopes.size() > 1) {
    const auto &parentScope = scopes[scopes.size() - 2];

    // 遍历上一级作用域中的所有符号
    for (const auto &entry : parentScope) {
      for (const auto &symbol : entry.second) {
        result.push_back(symbol);
      }
    }
  }

  return result;
}

// 获取所有符号（用于模块导入）
std::unordered_map<std::string, std::shared_ptr<Symbol>> SymbolTable::getAllSymbols() const {
  std::unordered_map<std::string, std::shared_ptr<Symbol>> result;

  // 从全局作用域开始收集符号
  for (const auto &scope : scopes) {
    for (const auto &entry : scope) {
      if (!entry.second.empty()) {
        result[entry.first] = entry.second[0];
      }
    }
  }

  return result;
}

} // namespace semantic
} // namespace c_hat
