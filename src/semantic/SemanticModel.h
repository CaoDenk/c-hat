#pragma once

#include "../ast/AstNodes.h"
#include "../types/Type.h"
#include <algorithm>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <vector>

namespace c_hat {
namespace semantic {

// 语义分析结果模型。
//
// 设计边界：AST 只保存语法，语义类型、调用目标、声明类型等信息由本模型
// 统一保存，codegen 只消费本模型，不再根据 LLVM 值反推类型。
class SemanticModel {
public:
  void setExpressionType(const ast::Expression *expression,
                         std::shared_ptr<types::Type> type) {
    if (expression && type) {
      expressionTypes_[expression] = std::move(type);
    }
  }

  std::shared_ptr<types::Type>
  getExpressionType(const ast::Expression *expression) const {
    if (!expression) {
      return nullptr;
    }
    auto it = expressionTypes_.find(expression);
    return it == expressionTypes_.end() ? nullptr : it->second;
  }

  void setDeclarationType(const ast::Declaration *declaration,
                          std::shared_ptr<types::Type> type) {
    if (declaration && type) {
      declarationTypes_[declaration] = std::move(type);
    }
  }

  std::shared_ptr<types::Type>
  getDeclarationType(const ast::Declaration *declaration) const {
    if (!declaration) {
      return nullptr;
    }
    auto it = declarationTypes_.find(declaration);
    return it == declarationTypes_.end() ? nullptr : it->second;
  }

  void dump(std::ostream &out) const {
    out << "=== Semantic Types ===\n";
    out << "declarations: " << declarationTypes_.size() << "\n";
    std::vector<std::pair<const ast::Declaration *, std::shared_ptr<types::Type>>>
        declarations(declarationTypes_.begin(), declarationTypes_.end());
    std::sort(declarations.begin(), declarations.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    for (const auto &[node, type] : declarations) {
      out << "  decl " << node << " : " << type->toString() << "\n";
    }
    out << "expressions: " << expressionTypes_.size() << "\n";
    std::vector<std::pair<const ast::Expression *, std::shared_ptr<types::Type>>>
        expressions(expressionTypes_.begin(), expressionTypes_.end());
    std::sort(expressions.begin(), expressions.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    for (const auto &[node, type] : expressions) {
      out << "  expr " << node << " : " << type->toString() << "\n";
    }
  }

private:
  std::unordered_map<const ast::Expression *, std::shared_ptr<types::Type>>
      expressionTypes_;
  std::unordered_map<const ast::Declaration *, std::shared_ptr<types::Type>>
      declarationTypes_;
};

} // namespace semantic
} // namespace c_hat