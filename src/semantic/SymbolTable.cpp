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
    scopes.back()[symbol->getName()] = symbol;
}

// 查找符号（从当前作用域开始向上查找）
std::shared_ptr<Symbol> SymbolTable::lookupSymbol(const std::string& name) {
    // 从当前作用域开始向上查找
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const auto& scope = *it;
        auto symbolIt = scope.find(name);
        if (symbolIt != scope.end()) {
            return symbolIt->second;
        }
    }
    
    // 未找到符号
    return nullptr;
}

// 检查当前作用域是否已存在该符号
bool SymbolTable::hasSymbolInCurrentScope(const std::string& name) const {
    if (scopes.empty()) {
        return false;
    }
    
    const auto& currentScope = scopes.back();
    return currentScope.find(name) != currentScope.end();
}

} // namespace semantic
} // namespace c_hat
