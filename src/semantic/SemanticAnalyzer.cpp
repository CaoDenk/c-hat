#include "SemanticAnalyzer.h"
#include "../ast/statements/BreakStmt.h"
#include "../ast/statements/ContinueStmt.h"
#include "../lexer/Lexer.h"
#include "EnumSymbol.h"
#include "ModuleSymbol.h"
#include "StructSymbol.h"
#include <format>
#include <iostream>

namespace c_hat {
namespace semantic {

// 辅助函数：将 specifiers 字符串转换为 Visibility 枚举
static Visibility parseVisibility(const std::string &specifiers) {
  if (specifiers == "public") {
    return Visibility::Public;
  } else if (specifiers == "private") {
    return Visibility::Private;
  } else if (specifiers == "protected") {
    return Visibility::Protected;
  } else if (specifiers == "internal") {
    return Visibility::Internal;
  }
  return Visibility::Default;
}

// SemanticAnalyzer 构造函数
SemanticAnalyzer::SemanticAnalyzer(const std::string &stdlibPath)
    : hasError_(false) {
  if (!stdlibPath.empty()) {
    moduleLoader_ = std::make_unique<ModuleLoader>(stdlibPath);
  } else {
    moduleLoader_ = std::make_unique<ModuleLoader>("stdlib");
  }
  // 初始化符号表，添加内置类型
  // 添加所有原始类型作为类型别名符号
  struct BuiltinType {
    std::string name;
    types::PrimitiveType::Kind kind;
  };

  std::vector<BuiltinType> builtins = {
      {"void", types::PrimitiveType::Kind::Void},
      {"bool", types::PrimitiveType::Kind::Bool},
      {"byte", types::PrimitiveType::Kind::Byte},
      {"sbyte", types::PrimitiveType::Kind::SByte},
      {"short", types::PrimitiveType::Kind::Short},
      {"ushort", types::PrimitiveType::Kind::UShort},
      {"int", types::PrimitiveType::Kind::Int},
      {"uint", types::PrimitiveType::Kind::UInt},
      {"long", types::PrimitiveType::Kind::Long},
      {"ulong", types::PrimitiveType::Kind::ULong},
      {"float", types::PrimitiveType::Kind::Float},
      {"double", types::PrimitiveType::Kind::Double},
      {"fp16", types::PrimitiveType::Kind::Fp16},
      {"bf16", types::PrimitiveType::Kind::Bf16},
      {"char", types::PrimitiveType::Kind::Char}};

  for (const auto &builtin : builtins) {
    auto type = types::TypeFactory::getPrimitiveType(builtin.kind);
    auto symbol = std::make_shared<TypeAliasSymbol>(builtin.name, type);
    symbolTable.addSymbol(symbol);
  }

  // 添加 LiteralView 类型
  auto literalViewType = types::TypeFactory::getLiteralViewType();
  auto literalViewSymbol =
      std::make_shared<TypeAliasSymbol>("LiteralView", literalViewType);
  symbolTable.addSymbol(literalViewSymbol);
}

// 分析整个程序
void SemanticAnalyzer::analyze(ast::Program &program) {
  currentProgram_ = &program;

  // 分析每个声明
  for (size_t i = 0; i < program.declarations.size(); ++i) {
    auto &declaration = program.declarations[i];
    analyzeDeclaration(declaration.get());
  }
}

// 分析声明
void SemanticAnalyzer::analyzeDeclaration(ast::Declaration *declaration) {
  switch (declaration->getType()) {
  case ast::NodeType::VariableDecl: {
    auto *varDecl = static_cast<ast::VariableDecl *>(declaration);
    analyzeVariableDecl(varDecl);
    break;
  }
  case ast::NodeType::TupleDestructuringDecl: {
    auto *tupleDecl = static_cast<ast::TupleDestructuringDecl *>(declaration);
    analyzeTupleDestructuringDecl(tupleDecl);
    break;
  }
  case ast::NodeType::FunctionDecl: {
    auto *funcDecl = static_cast<ast::FunctionDecl *>(declaration);
    analyzeFunctionDecl(funcDecl);
    break;
  }
  case ast::NodeType::ClassDecl: {
    auto *classDecl = static_cast<ast::ClassDecl *>(declaration);
    analyzeClassDecl(classDecl);
    break;
  }
  case ast::NodeType::StructDecl: {
    auto *structDecl = static_cast<ast::StructDecl *>(declaration);
    analyzeStructDecl(structDecl);
    break;
  }
  case ast::NodeType::EnumDecl: {
    auto *enumDecl = static_cast<ast::EnumDecl *>(declaration);
    analyzeEnumDecl(enumDecl);
    break;
  }
  case ast::NodeType::TypeAliasDecl: {
    auto *typeAliasDecl = static_cast<ast::TypeAliasDecl *>(declaration);
    analyzeTypeAliasDecl(typeAliasDecl);
    break;
  }
  case ast::NodeType::ModuleDecl: {
    auto *moduleDecl = static_cast<ast::ModuleDecl *>(declaration);
    analyzeModuleDecl(moduleDecl);
    break;
  }
  case ast::NodeType::ImportDecl: {
    auto *importDecl = static_cast<ast::ImportDecl *>(declaration);
    analyzeImportDecl(importDecl);
    break;
  }
  case ast::NodeType::ExtensionDecl: {
    auto *extensionDecl = static_cast<ast::ExtensionDecl *>(declaration);
    analyzeExtensionDecl(extensionDecl);
    break;
  }
  case ast::NodeType::GetterDecl: {
    auto *getterDecl = static_cast<ast::GetterDecl *>(declaration);
    analyzeGetterDecl(getterDecl);
    break;
  }
  case ast::NodeType::SetterDecl: {
    auto *setterDecl = static_cast<ast::SetterDecl *>(declaration);
    analyzeSetterDecl(setterDecl);
    break;
  }
  case ast::NodeType::ExternDecl: {
    auto *externDecl = static_cast<ast::ExternDecl *>(declaration);
    analyzeExternDecl(externDecl);
    break;
  }
  case ast::NodeType::NamespaceDecl: {
    auto *namespaceDecl = static_cast<ast::NamespaceDecl *>(declaration);
    analyzeNamespaceDecl(namespaceDecl);
    break;
  }
  default:
    // 未知的声明类型
    break;
  }
}

void SemanticAnalyzer::analyzeVariableDecl(ast::VariableDecl *varDecl) {
  // 分析变量类型
  std::shared_ptr<types::Type> varType = nullptr;
  if (varDecl->type) {
    if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
      varType = analyzeType(typeNode);
    }
  }

  // 分析初始化表达式
  if (varDecl->initializer) {
    std::shared_ptr<types::Type> initType =
        analyzeExpression(varDecl->initializer.get());

    // 如果变量没有显式类型，从初始化表达式推导类型
    if (!varType) {
      varType = initType;
    } else {
      // 检查类型兼容性
      if (!isTypeCompatible(varType, initType)) {
        error("type mismatch in variable initialization", *varDecl);
      }
    }
  }

  // 检查 late 变量
  if (varDecl->isLate) {
    // late 变量不能有初始化器
    if (varDecl->initializer) {
      error("late variable cannot have initializer", *varDecl);
    }

    // 记录 late 变量的状态
    lateVariables_[varDecl->name] = {false, varDecl};
  }

  // 添加到符号表
  auto varSymbol = std::make_shared<VariableSymbol>(
      varDecl->name, varType, varDecl->kind, varDecl->isConst);
  symbolTable.addSymbol(varSymbol);
}
void SemanticAnalyzer::analyzeTupleDestructuringDecl(
    ast::TupleDestructuringDecl *decl) {
  // 分析初始化表达式
  std::shared_ptr<types::Type> initType =
      analyzeExpression(decl->initializer.get());

  // 检查初始化表达式是否是 tuple 类型
  if (!initType || !initType->isTuple()) {
    error("Initializer for tuple destructuring must be a tuple", *decl);
    return;
  }

  // 检查变量名的数量与 tuple 元素的数量是否匹配
  auto *tupleType = static_cast<types::TupleType *>(initType.get());
  const auto &elementTypes = tupleType->getElementTypes();

  if (decl->names.size() != elementTypes.size()) {
    error("Number of variables in tuple destructuring must match number of "
          "tuple elements",
          *decl);
    return;
  }

  // 添加变量到符号表
  for (size_t i = 0; i < decl->names.size(); ++i) {
    const auto &name = decl->names[i];
    auto varSymbol = std::make_shared<VariableSymbol>(
        name, elementTypes[i], ast::VariableKind::Explicit, false);
    symbolTable.addSymbol(varSymbol);
  }
}
void SemanticAnalyzer::analyzeFunctionDecl(ast::FunctionDecl *funcDecl) {
  // 清空 late 变量表，因为它们的作用域是函数级别的
  lateVariables_.clear();

  // 分析函数返回类型
  std::shared_ptr<types::Type> returnType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
  if (funcDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
      returnType = analyzeType(typeNode);
    }
  }

  // 分析函数参数
  std::vector<std::shared_ptr<types::Type>> paramTypes;
  bool hasVariadicParam = false;
  for (size_t i = 0; i < funcDecl->params.size(); ++i) {
    auto &paramNode = funcDecl->params[i];
    if (auto *param = dynamic_cast<ast::Parameter *>(paramNode.get())) {
      if (param->type) {
        if (auto *typeNode = dynamic_cast<ast::Type *>(param->type.get())) {
          auto paramType = analyzeType(typeNode);
          paramTypes.push_back(paramType);
        }
      }
      if (param->isVariadic) {
        if (hasVariadicParam) {
          error("Only one variadic parameter is allowed", *paramNode);
        }
        if (i != funcDecl->params.size() - 1) {
          error("Variadic parameter must be the last parameter", *paramNode);
        }
        hasVariadicParam = true;
      }
    }
  }

  // 创建函数类型
  auto functionType =
      std::make_shared<types::FunctionType>(returnType, paramTypes);

  // 创建函数符号并添加到符号表
  auto funcSymbol =
      std::make_shared<FunctionSymbol>(funcDecl->name, functionType);
  symbolTable.addSymbol(funcSymbol);

  // 分析函数体
  if (funcDecl->body) {
    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      analyzeStatement(stmt);
    }
  }
}
void SemanticAnalyzer::analyzeClassDecl(ast::ClassDecl *classDecl) {}
void SemanticAnalyzer::analyzeStructDecl(ast::StructDecl *structDecl) {}
void SemanticAnalyzer::analyzeEnumDecl(ast::EnumDecl *enumDecl) {}
void SemanticAnalyzer::analyzeTypeAliasDecl(ast::TypeAliasDecl *typeAliasDecl) {
}
void SemanticAnalyzer::analyzeModuleDecl(ast::ModuleDecl *moduleDecl) {}
void SemanticAnalyzer::analyzeImportDecl(ast::ImportDecl *importDecl) {
  if (!currentProgram_) {
    return;
  }

  try {
    // 加载模块
    auto importedProgram = moduleLoader_->loadModule(importDecl->modulePath);

    if (importedProgram) {
      // 将导入模块的声明添加到当前程序中
      for (auto &decl : importedProgram->declarations) {
        currentProgram_->declarations.push_back(std::move(decl));
      }

      // 分析新添加的声明
      size_t startIdx = currentProgram_->declarations.size() -
                        importedProgram->declarations.size();
      for (size_t i = startIdx; i < currentProgram_->declarations.size(); ++i) {
        analyzeDeclaration(currentProgram_->declarations[i].get());
      }
    }
  } catch (const std::exception &e) {
    error("Failed to import module: " + std::string(e.what()), *importDecl);
  }
}
void SemanticAnalyzer::analyzeExtensionDecl(ast::ExtensionDecl *extensionDecl) {
  // 分析 extension 中的所有成员
  for (auto &member : extensionDecl->members) {
    if (auto *decl = dynamic_cast<ast::Declaration *>(member.get())) {
      analyzeDeclaration(decl);
    }
  }
}
void SemanticAnalyzer::analyzeGetterDecl(ast::GetterDecl *getterDecl) {}
void SemanticAnalyzer::analyzeSetterDecl(ast::SetterDecl *setterDecl) {}
void SemanticAnalyzer::analyzeExternDecl(ast::ExternDecl *externDecl) {}

void SemanticAnalyzer::analyzeNamespaceDecl(ast::NamespaceDecl *namespaceDecl) {
  // 进入命名空间作用域
  scopeStack.push_back(namespaceDecl->name);

  // 分析命名空间中的成员
  for (auto &member : namespaceDecl->members) {
    if (auto *decl = dynamic_cast<ast::Declaration *>(member.get())) {
      analyzeDeclaration(decl);
    }
  }

  // 退出命名空间作用域
  scopeStack.pop_back();
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeStatement(ast::Statement *stmt) {
  switch (stmt->getType()) {
  case ast::NodeType::VariableDecl: {
    auto *variableStmt = static_cast<ast::VariableStmt *>(stmt);
    analyzeVariableDecl(variableStmt->declaration.get());
    return nullptr;
  }
  case ast::NodeType::TupleDestructuringDecl: {
    auto *tupleDestructuringStmt =
        static_cast<ast::TupleDestructuringStmt *>(stmt);
    analyzeTupleDestructuringDecl(tupleDestructuringStmt->declaration.get());
    return nullptr;
  }
  case ast::NodeType::ExprStmt: {
    auto *exprStmt = static_cast<ast::ExprStmt *>(stmt);
    auto result = analyzeExprStmt(exprStmt);
    return result;
  }
  case ast::NodeType::CompoundStmt: {
    auto *compoundStmt = static_cast<ast::CompoundStmt *>(stmt);
    auto result = analyzeCompoundStmt(compoundStmt);
    return result;
  }
  case ast::NodeType::ReturnStmt: {
    auto *returnStmt = static_cast<ast::ReturnStmt *>(stmt);
    auto result = analyzeReturnStmt(returnStmt);
    return result;
  }
  case ast::NodeType::IfStmt: {
    auto *ifStmt = static_cast<ast::IfStmt *>(stmt);
    auto result = analyzeIfStmt(ifStmt);
    return result;
  }
  case ast::NodeType::WhileStmt: {
    auto *whileStmt = static_cast<ast::WhileStmt *>(stmt);
    auto result = analyzeWhileStmt(whileStmt);
    return result;
  }
  case ast::NodeType::ForStmt: {
    auto *forStmt = static_cast<ast::ForStmt *>(stmt);
    auto result = analyzeForStmt(forStmt);
    return result;
  }
  case ast::NodeType::BreakStmt: {
    auto *breakStmt = static_cast<ast::BreakStmt *>(stmt);
    auto result = analyzeBreakStmt(breakStmt);
    return result;
  }
  case ast::NodeType::ContinueStmt: {
    auto *continueStmt = static_cast<ast::ContinueStmt *>(stmt);
    auto result = analyzeContinueStmt(continueStmt);
    return result;
  }
  case ast::NodeType::MatchStmt: {
    auto *matchStmt = static_cast<ast::MatchStmt *>(stmt);
    auto result = analyzeMatchStmt(matchStmt);
    return result;
  }
  case ast::NodeType::TryStmt: {
    auto *tryStmt = static_cast<ast::TryStmt *>(stmt);
    auto result = analyzeTryStmt(tryStmt);
    return result;
  }
  case ast::NodeType::ThrowStmt: {
    auto *throwStmt = static_cast<ast::ThrowStmt *>(stmt);
    auto result = analyzeThrowStmt(throwStmt);
    return result;
  }
  case ast::NodeType::DeferStmt: {
    auto *deferStmt = static_cast<ast::DeferStmt *>(stmt);
    auto result = analyzeDeferStmt(deferStmt);
    return result;
  }
  default:
    return nullptr;
  }
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeExprStmt(ast::ExprStmt *exprStmt) {
  // 分析表达式
  auto result = analyzeExpression(exprStmt->expr.get());
  return result;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeCompoundStmt(ast::CompoundStmt *compoundStmt) {
  // 分析复合语句中的每个语句
  for (size_t i = 0; i < compoundStmt->statements.size(); ++i) {
    auto &stmt = compoundStmt->statements[i];
    if (auto *decl = dynamic_cast<ast::Declaration *>(stmt.get())) {
      // 直接使用原始指针调用 analyzeDeclaration 函数
      switch (decl->getType()) {
      case ast::NodeType::VariableDecl: {
        auto *varDecl = static_cast<ast::VariableDecl *>(decl);
        analyzeVariableDecl(varDecl);
        break;
      }
      case ast::NodeType::TupleDestructuringDecl: {
        auto *tupleDecl = static_cast<ast::TupleDestructuringDecl *>(decl);
        analyzeTupleDestructuringDecl(tupleDecl);
        break;
      }
      case ast::NodeType::FunctionDecl: {
        auto *funcDecl = static_cast<ast::FunctionDecl *>(decl);
        analyzeFunctionDecl(funcDecl);
        break;
      }
      default:
        break;
      }
    } else if (auto *stmtNode = dynamic_cast<ast::Statement *>(stmt.get())) {
      analyzeStatement(stmtNode);
    }
  }
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeReturnStmt(ast::ReturnStmt *returnStmt) {
  // 分析返回表达式
  std::shared_ptr<types::Type> result = nullptr;
  if (returnStmt->expr) {
    result = analyzeExpression(returnStmt->expr.get());
  }
  return result;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeIfStmt(ast::IfStmt *ifStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeWhileStmt(ast::WhileStmt *whileStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeForStmt(ast::ForStmt *forStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeBreakStmt(ast::BreakStmt *breakStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeContinueStmt(ast::ContinueStmt *continueStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeMatchStmt(ast::MatchStmt *matchStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTryStmt(ast::TryStmt *tryStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeThrowStmt(ast::ThrowStmt *throwStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeDeferStmt(ast::DeferStmt *deferStmt) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeExpression(ast::Expression *expression) {
  switch (expression->getType()) {
  case ast::NodeType::BinaryExpr: {
    auto *binaryExpr = static_cast<ast::BinaryExpr *>(expression);
    auto result = analyzeBinaryExpr(binaryExpr);
    return result;
  }
  case ast::NodeType::UnaryExpr: {
    auto *unaryExpr = static_cast<ast::UnaryExpr *>(expression);
    auto result = analyzeUnaryExpr(unaryExpr);
    return result;
  }
  case ast::NodeType::Identifier: {
    auto *identifier = static_cast<ast::Identifier *>(expression);
    auto result = analyzeIdentifierExpr(identifier);
    return result;
  }
  case ast::NodeType::Literal: {
    auto *literal = static_cast<ast::Literal *>(expression);
    auto result = analyzeLiteralExpr(literal);
    return result;
  }
  case ast::NodeType::CallExpr: {
    auto *callExpr = static_cast<ast::CallExpr *>(expression);
    auto result = analyzeCallExpr(callExpr);
    return result;
  }
  case ast::NodeType::MemberExpr: {
    auto *memberExpr = static_cast<ast::MemberExpr *>(expression);
    auto result = analyzeMemberExpr(memberExpr);
    return result;
  }
  case ast::NodeType::TupleExpr: {
    auto *tupleExpr = static_cast<ast::TupleExpr *>(expression);
    auto result = analyzeTupleExpr(tupleExpr);
    return result;
  }
  default:
    return nullptr;
  }
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeBinaryExpr(ast::BinaryExpr *binaryExpr) {
  // 保存左操作数的指针，用于后续的赋值操作检查
  ast::Expression *leftExpr = binaryExpr->left.get();

  // 分析左操作数
  std::shared_ptr<types::Type> leftType =
      analyzeExpression(binaryExpr->left.get());

  // 分析右操作数
  std::shared_ptr<types::Type> rightType =
      analyzeExpression(binaryExpr->right.get());

  // 处理赋值操作
  if (binaryExpr->op == ast::BinaryExpr::Op::Assign) {
    // 检查左操作数是否是标识符（变量）
    if (auto *identifier = dynamic_cast<ast::Identifier *>(leftExpr)) {
      const std::string &varName = identifier->name;

      // 检查变量是否是 let 声明的（不可重新赋值）
      auto symbol = symbolTable.lookupSymbol(varName);
      if (auto varSymbol = std::dynamic_pointer_cast<VariableSymbol>(symbol)) {
        if (varSymbol->isImmutableBinding()) {
          error("cannot assign to immutable variable '" + varName + "'",
                *binaryExpr);
        }
      }

      // 检查是否是 late 变量
      auto it = lateVariables_.find(varName);
      if (it != lateVariables_.end()) {
        // 标记为已初始化
        it->second.isInitialized = true;
      }
    }
  }

  // 检查类型兼容性
  if (leftType && rightType) {
    if (!isTypeCompatible(leftType, rightType)) {
      error("type mismatch in binary expression", *binaryExpr);
    }
  }

  return leftType;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeUnaryExpr(ast::UnaryExpr *unaryExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeIdentifierExpr(ast::Identifier *identifier) {
  const std::string &name = identifier->name;

  // 检查是否是 late 变量
  auto it = lateVariables_.find(name);
  if (it != lateVariables_.end()) {
    const LateVariableStatus &status = it->second;
    if (!status.isInitialized) {
      error("use of uninitialized late variable '" + name + "'", *identifier);
    }
  }

  // 从符号表获取类型
  auto symbol = symbolTable.lookupSymbol(name);
  if (auto varSymbol = std::dynamic_pointer_cast<VariableSymbol>(symbol)) {
    return varSymbol->getType();
  }

  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeLiteralExpr(ast::Literal *literal) {
  switch (literal->type) {
  case ast::Literal::Type::Integer:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Int);
  case ast::Literal::Type::Floating:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Float);
  case ast::Literal::Type::Boolean:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Bool);
  case ast::Literal::Type::String:
    // 字符串类型需要特殊处理，暂时返回 nullptr
    return nullptr;
  default:
    return nullptr;
  }
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeCallExpr(ast::CallExpr *callExpr) {
  // 分析函数名
  std::string funcName;
  if (auto *identifier =
          dynamic_cast<ast::Identifier *>(callExpr->callee.get())) {
    funcName = identifier->name;
  } else {
    // 暂时只处理直接函数调用
    error("Only direct function calls are supported", *callExpr);
    return nullptr;
  }

  // 分析实参类型
  std::vector<std::shared_ptr<types::Type>> argTypes;
  for (auto &arg : callExpr->args) {
    auto argType = analyzeExpression(arg.get());
    argTypes.push_back(argType);
  }

  // 查找所有同名函数符号
  auto candidateFuncs = symbolTable.lookupFunctionSymbols(funcName);
  if (candidateFuncs.empty()) {
    error("Function not found: " + funcName, *callExpr);
    return nullptr;
  }

  // 过滤和排序候选函数
  struct Candidate {
    std::shared_ptr<FunctionSymbol> func;
    int score; // 0: 精确匹配, 1: 需要隐式转换, -1: 不匹配
  };

  std::vector<Candidate> candidates;

  for (auto &func : candidateFuncs) {
    auto funcType = func->getType();
    if (!funcType)
      continue;

    auto &paramTypes = funcType->getParameterTypes();

    // 检查参数数量
    if (paramTypes.size() != argTypes.size()) {
      continue;
    }

    // 检查参数类型匹配
    int score = 0;
    bool match = true;

    for (size_t i = 0; i < argTypes.size(); ++i) {
      if (i >= paramTypes.size()) {
        // 可变参数，默认匹配
        continue;
      }

      auto argType = argTypes[i];
      auto paramType = paramTypes[i];

      if (!argType || !paramType) {
        // 如果参数类型或实参类型为 null，不匹配
        match = false;
        break;
      }

      if (argType->toString() == paramType->toString()) {
        // 精确匹配
        continue;
      } else {
        // 检查是否存在隐式转换
        // 暂时只支持简单的隐式转换检查
        score = 1;
      }
    }

    if (match) {
      // 只有匹配的函数才添加到候选列表中
      candidates.push_back({func, score});
    }
  }

  if (candidates.empty()) {
    error("No matching function found for: " + funcName, *callExpr);
    return nullptr;
  }

  // 选择最佳匹配
  // 首先按分数排序（精确匹配优先）
  std::sort(
      candidates.begin(), candidates.end(),
      [](const Candidate &a, const Candidate &b) { return a.score < b.score; });

  // 检查是否存在歧义
  if (candidates.size() > 1 && candidates[0].score == candidates[1].score) {
    error("Ambiguous function call: " + funcName, *callExpr);
    return nullptr;
  }

  // 返回最佳匹配函数的返回类型
  auto bestFunc = candidates[0].func;
  if (!bestFunc || !bestFunc->getType()) {
    return nullptr;
  }
  return bestFunc->getType()->getReturnType();
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeMemberExpr(ast::MemberExpr *memberExpr) {
  // 保存对象表达式的指针，用于后续的 late 变量检查
  ast::Expression *objectExpr = memberExpr->object.get();

  // 分析对象表达式
  std::shared_ptr<types::Type> objType =
      analyzeExpression(memberExpr->object.get());

  // 检查对象是否是 late 变量
  if (auto *identifier = dynamic_cast<ast::Identifier *>(objectExpr)) {
    const std::string &varName = identifier->name;
    auto it = lateVariables_.find(varName);
    if (it != lateVariables_.end()) {
      const LateVariableStatus &status = it->second;
      if (!status.isInitialized) {
        error("use of uninitialized late variable '" + varName + "'",
              *memberExpr);
      }
    }
  }

  return objType;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSubscriptExpr(ast::SubscriptExpr *subscriptExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeNewExpr(ast::NewExpr *newExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeDeleteExpr(ast::DeleteExpr *deleteExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeThisExpr(ast::ThisExpr *thisExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSuperExpr(ast::SuperExpr *superExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSelfExpr(ast::SelfExpr *selfExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeExpansionExpr(ast::ExpansionExpr *expansionExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeLambdaExpr(ast::LambdaExpr *lambdaExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeArrayInitExpr(ast::ArrayInitExpr *arrayInitExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeStructInitExpr(ast::StructInitExpr *structInitExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTupleExpr(ast::TupleExpr *tupleExpr) {
  std::vector<std::shared_ptr<types::Type>> elementTypes;

  // 分析每个元素
  for (size_t i = 0; i < tupleExpr->elements.size(); ++i) {
    std::shared_ptr<types::Type> elementType =
        analyzeExpression(tupleExpr->elements[i].get());
    elementTypes.push_back(elementType);
  }

  auto result = std::make_shared<types::TupleType>(elementTypes);
  return result;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeType(const ast::Type *type) {
  if (auto *primitiveType = dynamic_cast<const ast::PrimitiveType *>(type)) {
    return analyzePrimitiveType(primitiveType);
  } else if (auto *pointerType = dynamic_cast<const ast::PointerType *>(type)) {
    return analyzePointerType(pointerType);
  } else if (auto *arrayType = dynamic_cast<const ast::ArrayType *>(type)) {
    return analyzeArrayType(arrayType);
  } else if (auto *sliceType = dynamic_cast<const ast::SliceType *>(type)) {
    return analyzeSliceType(sliceType);
  } else if (auto *referenceType =
                 dynamic_cast<const ast::ReferenceType *>(type)) {
    return analyzeReferenceType(referenceType);
  } else if (auto *functionType =
                 dynamic_cast<const ast::FunctionType *>(type)) {
    return analyzeFunctionType(functionType);
  } else if (auto *namedType = dynamic_cast<const ast::NamedType *>(type)) {
    return analyzeNamedType(namedType);
  } else if (auto *tupleType = dynamic_cast<const ast::TupleType *>(type)) {
    return analyzeTupleType(tupleType);
  }
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzePrimitiveType(
    const ast::PrimitiveType *primitiveType) {
  // 将 ast::PrimitiveType::Kind 转换为 types::PrimitiveType::Kind
  types::PrimitiveType::Kind kind;
  switch (primitiveType->kind) {
  case ast::PrimitiveType::Kind::Void:
    kind = types::PrimitiveType::Kind::Void;
    break;
  case ast::PrimitiveType::Kind::Bool:
    kind = types::PrimitiveType::Kind::Bool;
    break;
  case ast::PrimitiveType::Kind::Byte:
    kind = types::PrimitiveType::Kind::Byte;
    break;
  case ast::PrimitiveType::Kind::SByte:
    kind = types::PrimitiveType::Kind::SByte;
    break;
  case ast::PrimitiveType::Kind::Short:
    kind = types::PrimitiveType::Kind::Short;
    break;
  case ast::PrimitiveType::Kind::UShort:
    kind = types::PrimitiveType::Kind::UShort;
    break;
  case ast::PrimitiveType::Kind::Int:
    kind = types::PrimitiveType::Kind::Int;
    break;
  case ast::PrimitiveType::Kind::UInt:
    kind = types::PrimitiveType::Kind::UInt;
    break;
  case ast::PrimitiveType::Kind::Long:
    kind = types::PrimitiveType::Kind::Long;
    break;
  case ast::PrimitiveType::Kind::ULong:
    kind = types::PrimitiveType::Kind::ULong;
    break;
  case ast::PrimitiveType::Kind::Float:
    kind = types::PrimitiveType::Kind::Float;
    break;
  case ast::PrimitiveType::Kind::Double:
    kind = types::PrimitiveType::Kind::Double;
    break;
  case ast::PrimitiveType::Kind::Fp16:
    kind = types::PrimitiveType::Kind::Fp16;
    break;
  case ast::PrimitiveType::Kind::Bf16:
    kind = types::PrimitiveType::Kind::Bf16;
    break;
  case ast::PrimitiveType::Kind::Char:
    kind = types::PrimitiveType::Kind::Char;
    break;
  default:
    return nullptr;
  }
  return types::TypeFactory::getPrimitiveType(kind);
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzePointerType(const ast::PointerType *pointerType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeArrayType(const ast::ArrayType *arrayType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSliceType(const ast::SliceType *sliceType) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeReferenceType(
    const ast::ReferenceType *referenceType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeFunctionType(const ast::FunctionType *functionType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeNamedType(const ast::NamedType *namedType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTupleType(const ast::TupleType *tupleType) {
  std::vector<std::shared_ptr<types::Type>> elementTypes;

  for (auto &elemType : tupleType->elementTypes) {
    std::shared_ptr<types::Type> elementType = analyzeType(elemType.get());
    elementTypes.push_back(elementType);
  }

  return std::make_shared<types::TupleType>(elementTypes);
}

bool SemanticAnalyzer::isLValue(const ast::Expression &expr) const {
  return false;
}
bool SemanticAnalyzer::isTypeCompatible(
    const std::shared_ptr<types::Type> &expected,
    const std::shared_ptr<types::Type> &actual) {
  if (!expected || !actual) {
    return false;
  }

  // 检查是否都是 tuple 类型
  if (expected->isTuple() && actual->isTuple()) {
    auto *expectedTuple = static_cast<types::TupleType *>(expected.get());
    auto *actualTuple = static_cast<types::TupleType *>(actual.get());

    // 检查元素数量是否相同
    const auto &expectedElements = expectedTuple->getElementTypes();
    const auto &actualElements = actualTuple->getElementTypes();

    if (expectedElements.size() != actualElements.size()) {
      return false;
    }

    // 检查每个元素的类型是否兼容
    for (size_t i = 0; i < expectedElements.size(); ++i) {
      if (!isTypeCompatible(expectedElements[i], actualElements[i])) {
        return false;
      }
    }

    return true;
  }

  // 检查是否是相同类型
  return expected->toString() == actual->toString();
}
std::shared_ptr<types::Type> SemanticAnalyzer::tryImplicitConversion(
    const std::shared_ptr<types::Type> &expected,
    const std::shared_ptr<types::Type> &actual) {
  return nullptr;
}
Visibility
SemanticAnalyzer::parseVisibility(const std::vector<std::string> &specifiers) {
  return Visibility::Default;
}
void SemanticAnalyzer::error(const std::string &message,
                             const ast::Node &node) {
  hasError_ = true;
}
void SemanticAnalyzer::initializeBuiltinSymbols() {}

} // namespace semantic
} // namespace c_hat
