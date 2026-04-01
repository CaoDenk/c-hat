#pragma once

#include "ClassSymbol.h"
#include "EnumSymbol.h"
#include "FunctionSymbol.h"
#include "StructSymbol.h"
#include "Symbol.h"
#include "TypeAliasSymbol.h"
#include "VariableSymbol.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace c_hat {
namespace semantic {

// 符号表类
class SymbolTable {
public:
  // 构造函数
  SymbolTable();

  // 进入一个新的作用域
  void enterScope();

  // 退出当前作用域
  void exitScope();

  // 获取当前作用域级别
  int getCurrentScopeLevel() const { return currentScopeLevel; }

  // 添加符号
  void addSymbol(std::shared_ptr<Symbol> symbol);

  // 查找符号（从当前作用域开始向上查找）
  std::shared_ptr<Symbol> lookupSymbol(const std::string &name);

  // 查找所有同名函数符号（从当前作用域开始向上查找）
  std::vector<std::shared_ptr<FunctionSymbol>> lookupFunctionSymbols(const std::string &name);

  // 检查当前作用域是否已存在该符号
  bool hasSymbolInCurrentScope(const std::string &name) const;

  // 移除符号（用于方法重写时移除继承的方法）
  void removeSymbol(const std::string &name, std::shared_ptr<Symbol> symbol);

  // 获取上一级作用域中的所有符号
  std::vector<std::shared_ptr<Symbol>> getSymbolsInParentScope() const;

  // 获取所有符号（用于模块导入）
  std::unordered_map<std::string, std::shared_ptr<Symbol>> getAllSymbols() const;

private:
  // 当前作用域级别
  int currentScopeLevel;

  // 符号表栈，每个作用域对应一个符号表
  std::vector<std::unordered_map<std::string, std::vector<std::shared_ptr<Symbol>>>> scopes;
};

} // namespace semantic
} // namespace c_hat
