#pragma once

#include <string>

namespace c_hat {
namespace semantic {

// 可见性枚举
enum class Visibility {
  Default, // 默认可见性（通常是 module 或 private，取决于上下文）
  Public,
  Private,
  Protected,
  Internal
};

// 符号类型枚举
enum class SymbolType {
  Variable,
  Function,
  Class,
  Interface,
  Struct,
  Enum,
  Union,
  TypeAlias,
  Module
};

// 符号基类
class Symbol {
public:
  virtual ~Symbol() = default;

  // 获取符号名称
  const std::string &getName() const { return name; }

  // 获取符号类型
  SymbolType getType() const { return type; }

  // 获取符号所在作用域
  int getScopeLevel() const { return scopeLevel; }

  // 设置符号所在作用域
  void setScopeLevel(int level) { scopeLevel = level; }

  // 获取可见性
  Visibility getVisibility() const { return visibility; }

  // 设置可见性
  void setVisibility(Visibility vis) { visibility = vis; }

protected:
  Symbol(const std::string &name, SymbolType type,
         Visibility visibility = Visibility::Default)
      : name(name), type(type), scopeLevel(0), visibility(visibility) {}

private:
  std::string name;
  SymbolType type;
  int scopeLevel;
  Visibility visibility;
};

} // namespace semantic
} // namespace c_hat
