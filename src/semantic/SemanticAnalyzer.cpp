#include "SemanticAnalyzer.h"
#include "../ast/AstNodes.h"
#include "../types/ClassType.h"
#include "../types/InterfaceType.h"
#include "../types/TypeFactory.h"
#include <iostream>

namespace c_hat {
namespace semantic {

SemanticAnalyzer::SemanticAnalyzer(const std::string &stdlibPath) {
  (void)stdlibPath; // 暂时不使用
  initializeBuiltinSymbols();
}

void SemanticAnalyzer::analyze(ast::Program &program) {
  currentProgram_ = &program;

  // 第一遍：收集所有声明
  for (auto &decl : program.declarations) {
    if (auto *varDecl = dynamic_cast<ast::VariableDecl *>(decl.get())) {
      // 处理变量声明
    } else if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(decl.get())) {
      // 处理函数声明
      analyzeFunctionDecl(funcDecl);
    } else if (auto *classDecl = dynamic_cast<ast::ClassDecl *>(decl.get())) {
      analyzeClassDecl(classDecl);
    } else if (auto *interfaceDecl =
                   dynamic_cast<ast::InterfaceDecl *>(decl.get())) {
      analyzeInterfaceDecl(interfaceDecl);
    }
  }

  // 第二遍：分析函数体
  for (auto &decl : program.declarations) {
    if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(decl.get())) {
      if (funcDecl->body) {
        if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
          analyzeStatement(stmt);
        }
      }
    }
  }
}

void SemanticAnalyzer::initializeBuiltinSymbols() {
  // 添加基本类型到符号表
  auto intType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int);
  auto floatType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Float);
  auto doubleType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Double);
  auto boolType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Bool);
  auto charType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Char);
  auto voidType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);

  // 添加 string 类型到符号表
  auto stringType = std::make_shared<types::ClassType>("string");
  auto stringSymbol = std::make_shared<ClassSymbol>("string", stringType);
  symbolTable.addSymbol(stringSymbol);
}

void SemanticAnalyzer::analyzeDeclaration(ast::Declaration *declaration) {
  switch (declaration->getType()) {
  case ast::NodeType::VariableDecl:
    analyzeVariableDecl(static_cast<ast::VariableDecl *>(declaration));
    break;
  case ast::NodeType::FunctionDecl:
    analyzeFunctionDecl(static_cast<ast::FunctionDecl *>(declaration));
    break;
  case ast::NodeType::ClassDecl:
    analyzeClassDecl(static_cast<ast::ClassDecl *>(declaration));
    break;
  case ast::NodeType::InterfaceDecl:
    analyzeInterfaceDecl(static_cast<ast::InterfaceDecl *>(declaration));
    break;
  case ast::NodeType::StructDecl:
    analyzeStructDecl(static_cast<ast::StructDecl *>(declaration));
    break;
  case ast::NodeType::EnumDecl:
    analyzeEnumDecl(static_cast<ast::EnumDecl *>(declaration));
    break;
  case ast::NodeType::TypeAliasDecl:
    analyzeTypeAliasDecl(static_cast<ast::TypeAliasDecl *>(declaration));
    break;
  case ast::NodeType::ModuleDecl:
    analyzeModuleDecl(static_cast<ast::ModuleDecl *>(declaration));
    break;
  case ast::NodeType::ImportDecl:
    analyzeImportDecl(static_cast<ast::ImportDecl *>(declaration));
    break;
  case ast::NodeType::ExtensionDecl:
    analyzeExtensionDecl(static_cast<ast::ExtensionDecl *>(declaration));
    break;
  case ast::NodeType::GetterDecl:
    analyzeGetterDecl(static_cast<ast::GetterDecl *>(declaration));
    break;
  case ast::NodeType::SetterDecl:
    analyzeSetterDecl(static_cast<ast::SetterDecl *>(declaration));
    break;
  case ast::NodeType::ExternDecl:
    analyzeExternDecl(static_cast<ast::ExternDecl *>(declaration));
    break;
  case ast::NodeType::NamespaceDecl:
    analyzeNamespaceDecl(static_cast<ast::NamespaceDecl *>(declaration));
    break;
  default:
    error("Unknown declaration type", *declaration);
  }
}

void SemanticAnalyzer::analyzeVariableDecl(ast::VariableDecl *varDecl) {}
void SemanticAnalyzer::analyzeTupleDestructuringDecl(
    ast::TupleDestructuringDecl *decl) {}

void SemanticAnalyzer::analyzeFunctionDecl(ast::FunctionDecl *funcDecl) {
  // 分析参数
  std::vector<std::shared_ptr<types::Type>> paramTypes;
  for (auto &param : funcDecl->params) {
    if (auto *paramNode = dynamic_cast<ast::Parameter *>(param.get())) {
      if (auto *typeNode = dynamic_cast<ast::Type *>(paramNode->type.get())) {
        auto paramType = analyzeType(typeNode);
        if (paramType) {
          paramTypes.push_back(paramType);
        }
      }
    }
  }

  // 分析返回类型
  std::shared_ptr<types::Type> returnType;
  if (funcDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
      returnType = analyzeType(typeNode);
    }
  } else {
    returnType =
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
  }

  // 创建函数类型
  auto funcType = std::make_shared<types::FunctionType>(returnType, paramTypes);

  // 创建函数符号并添加到符号表
  auto funcSymbol = std::make_shared<FunctionSymbol>(funcDecl->name, funcType);
  symbolTable.addSymbol(funcSymbol);

  // 进入函数作用域
  symbolTable.enterScope();

  // 添加参数到函数作用域
  for (auto &param : funcDecl->params) {
    if (auto *paramNode = dynamic_cast<ast::Parameter *>(param.get())) {
      if (auto *typeNode = dynamic_cast<ast::Type *>(paramNode->type.get())) {
        auto paramType = analyzeType(typeNode);
        if (paramType) {
          auto paramSymbol =
              std::make_shared<VariableSymbol>(paramNode->name, paramType);
          symbolTable.addSymbol(paramSymbol);
        }
      }
    }
  }

  // 分析函数体
  if (funcDecl->body) {
    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      analyzeStatement(stmt);
    }
  }

  // 退出函数作用域
  symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeClassDecl(ast::ClassDecl *classDecl) {
  // 创建类类型
  auto classType = std::make_shared<types::ClassType>(classDecl->name);

  // 创建类符号并添加到符号表
  auto classSymbol = std::make_shared<ClassSymbol>(classDecl->name, classType);
  symbolTable.addSymbol(classSymbol);

  // 处理基类（单继承）
  if (!classDecl->baseClass.empty()) {
    auto baseType = analyzeTypeByName(classDecl->baseClass);
    if (baseType && baseType->isClass()) {
      auto baseClassType =
          std::dynamic_pointer_cast<types::ClassType>(baseType);
      classType->addBaseClass(baseClassType);
    } else {
      error("Base class not found: " + classDecl->baseClass, *classDecl);
    }
  }

  // 处理接口
  for (const auto &interfaceName : classDecl->interfaces) {
    auto interfaceType = analyzeTypeByName(interfaceName);
    if (interfaceType && interfaceType->isInterface()) {
      auto interface =
          std::dynamic_pointer_cast<types::InterfaceType>(interfaceType);
      classType->addInterface(interface);
    } else {
      error("Interface not found: " + interfaceName, *classDecl);
    }
  }

  // 检查接口方法冲突
  auto conflicts = classType->detectInterfaceMethodConflicts();
  if (!conflicts.empty()) {
    for (const auto &methodName : conflicts) {
      error("Interface method conflict: " + methodName, *classDecl);
    }
  }

  // 进入类作用域
  symbolTable.enterScope();

  // 分析类成员
  for (auto &member : classDecl->members) {
    if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(member.get())) {
      // 分析方法声明
      analyzeFunctionDecl(funcDecl);

      // 解析访问控制修饰符
      auto visibility = parseVisibility({funcDecl->specifiers});
      auto access = visibilityToAccessModifier(visibility);

      // 将方法添加到类类型
      std::vector<std::shared_ptr<types::Type>> paramTypes;
      for (const auto &param : funcDecl->params) {
        if (auto *paramNode = dynamic_cast<ast::Parameter *>(param.get())) {
          if (auto *typeNode =
                  dynamic_cast<ast::Type *>(paramNode->type.get())) {
            auto paramType = analyzeType(typeNode);
            paramTypes.push_back(paramType);
          }
        }
      }

      std::shared_ptr<types::Type> returnType;
      if (funcDecl->returnType) {
        if (auto *typeNode =
                dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
          returnType = analyzeType(typeNode);
        }
      } else {
        returnType = types::TypeFactory::getPrimitiveType(
            types::PrimitiveType::Kind::Void);
      }

      // 检查是否是虚方法或重写方法
      bool isVirtual =
          (funcDecl->specifiers.find("virtual") != std::string::npos);
      bool isOverride =
          (funcDecl->specifiers.find("override") != std::string::npos);

      // 检查是否是构造函数或析构函数
      if (funcDecl->name == classDecl->name) {
        // 构造函数：名称与类名相同，没有返回类型
        if (funcDecl->returnType) {
          error("Constructor cannot have a return type", *funcDecl);
        }
      } else if (funcDecl->name == "~" + classDecl->name) {
        // 析构函数：名称是类名前面加上波浪号，没有返回类型，没有参数
        if (funcDecl->returnType) {
          error("Destructor cannot have a return type", *funcDecl);
        }
        if (!funcDecl->params.empty()) {
          error("Destructor cannot have parameters", *funcDecl);
        }
      }

      types::ClassMethod method(funcDecl->name, returnType, paramTypes,
                                isVirtual, isOverride, access);
      classType->addMethod(method);
    } else if (auto *varDecl =
                   dynamic_cast<ast::VariableDecl *>(member.get())) {
      // 分析字段声明
      analyzeVariableDecl(varDecl);

      // 解析访问控制修饰符
      auto visibility = parseVisibility({varDecl->specifiers});
      auto access = visibilityToAccessModifier(visibility);

      // 分析字段类型
      std::shared_ptr<types::Type> fieldType;
      if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
        fieldType = analyzeType(typeNode);
      }

      // 将字段添加到类类型
      types::ClassField field(varDecl->name, fieldType, access);
      classType->addField(field);
    } else if (auto *getterDecl =
                   dynamic_cast<ast::GetterDecl *>(member.get())) {
      // 分析 getter 声明
      analyzeGetterDecl(getterDecl);

      // 解析访问控制修饰符
      auto visibility = parseVisibility({getterDecl->specifiers});
      auto propertyAccess = visibilityToAccessModifier(visibility);

      // 分析属性类型
      std::shared_ptr<types::Type> propertyType;
      if (auto *typeNode =
              dynamic_cast<ast::Type *>(getterDecl->returnType.get())) {
        propertyType = analyzeType(typeNode);
        if (!propertyType) {
          error("Invalid property type", *getterDecl);
        }
      }

      // 检查属性是否已存在
      if (classType->hasProperty(getterDecl->name)) {
        auto *existingProperty = classType->getProperty(getterDecl->name);
        if (existingProperty->hasGetter) {
          error("Getter already defined for property: " + getterDecl->name,
                *getterDecl);
        } else {
          // 更新现有属性
          const_cast<types::ClassProperty *>(existingProperty)->hasGetter =
              true;
        }
      } else {
        // 创建新属性
        types::ClassProperty property(getterDecl->name, propertyType,
                                      propertyAccess);
        property.hasGetter = true;
        classType->addProperty(property);
      }
    } else if (auto *setterDecl =
                   dynamic_cast<ast::SetterDecl *>(member.get())) {
      // 分析 setter 声明
      analyzeSetterDecl(setterDecl);

      // 解析访问控制修饰符
      auto visibility = parseVisibility({setterDecl->specifiers});
      auto propertyAccess = visibilityToAccessModifier(visibility);

      // 分析属性类型
      std::shared_ptr<types::Type> propertyType;
      if (auto *paramNode =
              dynamic_cast<ast::Parameter *>(setterDecl->param.get())) {
        if (auto *typeNode = dynamic_cast<ast::Type *>(paramNode->type.get())) {
          propertyType = analyzeType(typeNode);
          if (!propertyType) {
            error("Invalid property type", *setterDecl);
          }
        }
      }

      // 检查属性是否已存在
      if (classType->hasProperty(setterDecl->name)) {
        auto *existingProperty = classType->getProperty(setterDecl->name);
        if (existingProperty->hasSetter) {
          error("Setter already defined for property: " + setterDecl->name,
                *setterDecl);
        } else {
          // 更新现有属性
          const_cast<types::ClassProperty *>(existingProperty)->hasSetter =
              true;
        }
      } else {
        // 创建新属性
        types::ClassProperty property(setterDecl->name, propertyType,
                                      propertyAccess);
        property.hasSetter = true;
        classType->addProperty(property);
      }
    } else if (auto *decl = dynamic_cast<ast::Declaration *>(member.get())) {
      analyzeDeclaration(decl);
    } else if (auto *stmt = dynamic_cast<ast::Statement *>(member.get())) {
      analyzeStatement(stmt);
    }
  }

  // 退出类作用域
  symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeInterfaceDecl(ast::InterfaceDecl *interfaceDecl) {
  // 创建接口类型
  auto interfaceType =
      std::make_shared<types::InterfaceType>(interfaceDecl->name);

  // 先添加到符号表，以便处理循环依赖
  auto interfaceSymbol =
      std::make_shared<ClassSymbol>(interfaceDecl->name, interfaceType);
  symbolTable.addSymbol(interfaceSymbol);

  // 处理父接口
  for (const auto &baseInterfaceName : interfaceDecl->baseInterfaces) {
    auto baseType = analyzeTypeByName(baseInterfaceName);
    if (baseType && baseType->isInterface()) {
      auto baseInterfaceType =
          std::dynamic_pointer_cast<types::InterfaceType>(baseType);
      interfaceType->addBaseInterface(baseInterfaceType);
    } else {
      error("Base interface not found: " + baseInterfaceName, *interfaceDecl);
    }
  }

  // 进入接口作用域
  symbolTable.enterScope();

  // 分析接口成员
  for (auto &member : interfaceDecl->members) {
    if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(member.get())) {
      // 分析方法声明
      analyzeFunctionDecl(funcDecl);

      // 将方法添加到接口类型
      std::vector<std::shared_ptr<types::Type>> paramTypes;
      for (const auto &param : funcDecl->params) {
        if (auto *paramNode = dynamic_cast<ast::Parameter *>(param.get())) {
          if (auto *typeNode =
                  dynamic_cast<ast::Type *>(paramNode->type.get())) {
            auto paramType = analyzeType(typeNode);
            paramTypes.push_back(paramType);
          }
        }
      }

      std::shared_ptr<types::Type> returnType;
      if (funcDecl->returnType) {
        if (auto *typeNode =
                dynamic_cast<ast::Type *>(funcDecl->returnType.get())) {
          returnType = analyzeType(typeNode);
        }
      } else {
        returnType = types::TypeFactory::getPrimitiveType(
            types::PrimitiveType::Kind::Void);
      }

      // 检查是否有方法体（默认实现）
      bool hasDefaultImpl = (funcDecl->body != nullptr);

      // 解析访问控制修饰符
      auto visibility = parseVisibility({funcDecl->specifiers});
      auto access = visibilityToAccessModifier(visibility);

      types::InterfaceMethod method(funcDecl->name, returnType, paramTypes,
                                    hasDefaultImpl, access);
      interfaceType->addMethod(method);
    } else if (auto *decl = dynamic_cast<ast::Declaration *>(member.get())) {
      analyzeDeclaration(decl);
    } else if (auto *stmt = dynamic_cast<ast::Statement *>(member.get())) {
      analyzeStatement(stmt);
    }
  }

  // 退出接口作用域
  symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeStructDecl(ast::StructDecl *structDecl) {}
void SemanticAnalyzer::analyzeEnumDecl(ast::EnumDecl *enumDecl) {}
void SemanticAnalyzer::analyzeTypeAliasDecl(ast::TypeAliasDecl *typeAliasDecl) {
}
void SemanticAnalyzer::analyzeModuleDecl(ast::ModuleDecl *moduleDecl) {}
void SemanticAnalyzer::analyzeImportDecl(ast::ImportDecl *importDecl) {}
void SemanticAnalyzer::analyzeExtensionDecl(ast::ExtensionDecl *extensionDecl) {
}
void SemanticAnalyzer::analyzeGetterDecl(ast::GetterDecl *getterDecl) {
  // 分析返回类型
  if (getterDecl->returnType) {
    if (auto *typeNode =
            dynamic_cast<ast::Type *>(getterDecl->returnType.get())) {
      auto type = analyzeType(typeNode);
      if (!type) {
        error("Invalid property type", *getterDecl);
      }
    }
  }

  // 分析方法体
  if (getterDecl->body) {
    if (auto *compoundStmt =
            dynamic_cast<ast::CompoundStmt *>(getterDecl->body.get())) {
      analyzeStatement(compoundStmt);
    }
  }

  // 分析箭头表达式（如果有）
  if (getterDecl->arrowExpr) {
    analyzeExpression(getterDecl->arrowExpr.get());
  }
}

void SemanticAnalyzer::analyzeSetterDecl(ast::SetterDecl *setterDecl) {
  // 分析参数
  if (setterDecl->param) {
    if (auto *paramNode =
            dynamic_cast<ast::Parameter *>(setterDecl->param.get())) {
      if (auto *typeNode = dynamic_cast<ast::Type *>(paramNode->type.get())) {
        auto type = analyzeType(typeNode);
        if (!type) {
          error("Invalid parameter type", *setterDecl);
        }
      }
    }
  }

  // 分析方法体
  if (setterDecl->body) {
    if (auto *compoundStmt =
            dynamic_cast<ast::CompoundStmt *>(setterDecl->body.get())) {
      analyzeStatement(compoundStmt);
    }
  }

  // 分析箭头表达式（如果有）
  if (setterDecl->arrowExpr) {
    analyzeExpression(setterDecl->arrowExpr.get());
  }
}
void SemanticAnalyzer::analyzeExternDecl(ast::ExternDecl *externDecl) {}
void SemanticAnalyzer::analyzeNamespaceDecl(ast::NamespaceDecl *namespaceDecl) {
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeStatement(ast::Statement *stmt) {
  switch (stmt->getType()) {
  case ast::NodeType::ExprStmt:
    return analyzeExprStmt(static_cast<ast::ExprStmt *>(stmt));
  case ast::NodeType::CompoundStmt:
    return analyzeCompoundStmt(static_cast<ast::CompoundStmt *>(stmt));
  case ast::NodeType::IfStmt:
    return analyzeIfStmt(static_cast<ast::IfStmt *>(stmt));
  case ast::NodeType::WhileStmt:
    return analyzeWhileStmt(static_cast<ast::WhileStmt *>(stmt));
  case ast::NodeType::ForStmt:
    return analyzeForStmt(static_cast<ast::ForStmt *>(stmt));
  case ast::NodeType::ReturnStmt:
    return analyzeReturnStmt(static_cast<ast::ReturnStmt *>(stmt));
  case ast::NodeType::BreakStmt:
    return analyzeBreakStmt(static_cast<ast::BreakStmt *>(stmt));
  case ast::NodeType::ContinueStmt:
    return analyzeContinueStmt(static_cast<ast::ContinueStmt *>(stmt));
  case ast::NodeType::MatchStmt:
    return analyzeMatchStmt(static_cast<ast::MatchStmt *>(stmt));
  case ast::NodeType::TryStmt:
    return analyzeTryStmt(static_cast<ast::TryStmt *>(stmt));
  case ast::NodeType::ThrowStmt:
    return analyzeThrowStmt(static_cast<ast::ThrowStmt *>(stmt));
  case ast::NodeType::DeferStmt:
    return analyzeDeferStmt(static_cast<ast::DeferStmt *>(stmt));
  default:
    error("Unknown statement type", *stmt);
    return nullptr;
  }
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeExprStmt(ast::ExprStmt *exprStmt) {
  if (exprStmt->expr) {
    return analyzeExpression(exprStmt->expr.get());
  }
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeCompoundStmt(ast::CompoundStmt *compoundStmt) {
  symbolTable.enterScope();
  std::shared_ptr<types::Type> lastType;
  for (auto &stmt : compoundStmt->statements) {
    if (auto *statement = dynamic_cast<ast::Statement *>(stmt.get())) {
      lastType = analyzeStatement(statement);
    } else if (auto *decl = dynamic_cast<ast::Declaration *>(stmt.get())) {
      analyzeDeclaration(decl);
    }
  }
  symbolTable.exitScope();
  return lastType;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeReturnStmt(ast::ReturnStmt *returnStmt) {
  if (returnStmt->expr) {
    return analyzeExpression(returnStmt->expr.get());
  }
  return types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
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
  case ast::NodeType::BinaryExpr:
    return analyzeBinaryExpr(static_cast<ast::BinaryExpr *>(expression));
  case ast::NodeType::UnaryExpr:
    return analyzeUnaryExpr(static_cast<ast::UnaryExpr *>(expression));
  case ast::NodeType::Identifier:
    return analyzeIdentifierExpr(static_cast<ast::Identifier *>(expression));
  case ast::NodeType::Literal:
    return analyzeLiteralExpr(static_cast<ast::Literal *>(expression));
  case ast::NodeType::CallExpr:
    return analyzeCallExpr(static_cast<ast::CallExpr *>(expression));
  case ast::NodeType::MemberExpr:
    return analyzeMemberExpr(static_cast<ast::MemberExpr *>(expression));
  case ast::NodeType::SubscriptExpr:
    return analyzeSubscriptExpr(static_cast<ast::SubscriptExpr *>(expression));
  case ast::NodeType::NewExpr:
    return analyzeNewExpr(static_cast<ast::NewExpr *>(expression));
  case ast::NodeType::DeleteExpr:
    return analyzeDeleteExpr(static_cast<ast::DeleteExpr *>(expression));
  case ast::NodeType::ThisExpr:
    return analyzeThisExpr(static_cast<ast::ThisExpr *>(expression));
  case ast::NodeType::SuperExpr:
    return analyzeSuperExpr(static_cast<ast::SuperExpr *>(expression));
  case ast::NodeType::SelfExpr:
    return analyzeSelfExpr(static_cast<ast::SelfExpr *>(expression));
  case ast::NodeType::ExpansionExpr:
    return analyzeExpansionExpr(static_cast<ast::ExpansionExpr *>(expression));
  case ast::NodeType::LambdaExpr:
    return analyzeLambdaExpr(static_cast<ast::LambdaExpr *>(expression));
  case ast::NodeType::ArrayInitExpr:
    return analyzeArrayInitExpr(static_cast<ast::ArrayInitExpr *>(expression));
  case ast::NodeType::StructInitExpr:
    return analyzeStructInitExpr(
        static_cast<ast::StructInitExpr *>(expression));
  case ast::NodeType::TupleExpr:
    return analyzeTupleExpr(static_cast<ast::TupleExpr *>(expression));
  default:
    error("Unknown expression type", *expression);
    return nullptr;
  }
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeBinaryExpr(ast::BinaryExpr *binaryExpr) {
  // 处理赋值操作
  if (binaryExpr->op == ast::BinaryExpr::Op::Assign) {
    // 分析左操作数（赋值目标）
    auto leftType = analyzeExpression(binaryExpr->left.get());

    // 分析右操作数（赋值值）
    auto rightType = analyzeExpression(binaryExpr->right.get());

    // 检查是否是属性赋值
    if (auto *memberExpr =
            dynamic_cast<ast::MemberExpr *>(binaryExpr->left.get())) {
      auto objectType = analyzeExpression(memberExpr->object.get());
      if (objectType && objectType->isClass()) {
        auto classType =
            std::dynamic_pointer_cast<types::ClassType>(objectType);

        // 检查是否是属性
        if (classType->hasProperty(memberExpr->member)) {
          const auto *property = classType->getProperty(memberExpr->member);
          if (property && !property->hasSetter) {
            error("Property has no setter: " + memberExpr->member, *binaryExpr);
            return nullptr;
          }
        }
      }
    }

    // 检查类型兼容性
    if (leftType && rightType) {
      if (!isTypeCompatible(leftType, rightType)) {
        error("Type mismatch in assignment", *binaryExpr);
        return nullptr;
      }
    }

    return leftType;
  }

  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeUnaryExpr(ast::UnaryExpr *unaryExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeIdentifierExpr(ast::Identifier *identifier) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeLiteralExpr(ast::Literal *literal) {
  // 根据字面量类型返回相应的类型
  switch (literal->type) {
  case ast::Literal::Type::Integer:
    // 默认为 int 类型
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Int);
  case ast::Literal::Type::Floating:
    // 默认为 double 类型
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Double);
  case ast::Literal::Type::Boolean:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Bool);
  case ast::Literal::Type::String:
    // 字符串字面量类型为 LiteralView
    // 暂时返回 void，需要在类型系统中添加 LiteralView 类型
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  default:
    return nullptr;
  }
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeCallExpr(ast::CallExpr *callExpr) {
  // 分析参数类型
  std::vector<std::shared_ptr<types::Type>> argTypes;
  for (auto &arg : callExpr->args) {
    auto argType = analyzeExpression(arg.get());
    if (!argType) {
      error("Invalid argument type", *arg);
      return nullptr;
    }
    argTypes.push_back(argType);
  }

  // 处理标识符调用的情况
  if (auto *identifier =
          dynamic_cast<ast::Identifier *>(callExpr->callee.get())) {
    // 从符号表中查找所有同名函数
    auto functionSymbols = symbolTable.lookupFunctionSymbols(identifier->name);
    if (functionSymbols.empty()) {
      error("Function not found: " + identifier->name, *callExpr);
      return nullptr;
    }

    // 收集所有可能的匹配
    std::vector<std::shared_ptr<FunctionSymbol>> viableCandidates;
    std::vector<std::shared_ptr<FunctionSymbol>> exactCandidates;

    for (const auto &funcSymbol : functionSymbols) {
      auto funcType = funcSymbol->getType();
      // 检查参数数量是否匹配
      if (funcType->getParameterTypes().size() != argTypes.size()) {
        continue;
      }

      // 检查每个参数是否兼容
      bool isViable = true;
      bool isExact = true;
      for (size_t i = 0; i < argTypes.size(); ++i) {
        const auto &paramType = funcType->getParameterTypes()[i];
        const auto &argType = argTypes[i];

        if (!isTypeCompatible(paramType, argType)) {
          isViable = false;
          break;
        }

        if (paramType->toString() != argType->toString()) {
          isExact = false;
        }
      }

      if (isViable) {
        viableCandidates.push_back(funcSymbol);
        if (isExact) {
          exactCandidates.push_back(funcSymbol);
        }
      }
    }

    // 根据规则选择
    if (exactCandidates.size() == 1) {
      // 规则 1：精确匹配优先
      return exactCandidates[0]->getType()->getReturnType();
    } else if (viableCandidates.size() == 1) {
      // 规则 2：单一隐式转换路径
      return viableCandidates[0]->getType()->getReturnType();
    } else if (viableCandidates.size() > 1) {
      // 规则 3：歧义时报错
      error("Ambiguous function call: " + identifier->name, *callExpr);
      return nullptr;
    } else {
      // 无匹配
      error("No matching function for: " + identifier->name, *callExpr);
      return nullptr;
    }
  }

  // 暂时返回 void 类型
  return types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeMemberExpr(ast::MemberExpr *memberExpr) {
  // 分析对象表达式
  auto objectType = analyzeExpression(memberExpr->object.get());
  if (!objectType) {
    error("Invalid object type in member expression", *memberExpr);
    return nullptr;
  }

  // 检查是否是类类型
  if (objectType->isClass()) {
    auto classType = std::dynamic_pointer_cast<types::ClassType>(objectType);

    // 检查字段是否存在
    if (classType->hasField(memberExpr->member)) {
      return classType->getFieldType(memberExpr->member);
    }

    // 检查属性是否存在
    if (classType->hasProperty(memberExpr->member)) {
      const auto *property = classType->getProperty(memberExpr->member);
      if (property && property->hasGetter) {
        return property->type;
      } else if (property && !property->hasGetter) {
        error("Property has no getter: " + memberExpr->member, *memberExpr);
        return nullptr;
      }
    }

    // 检查方法是否存在
    if (classType->hasMethod(memberExpr->member)) {
      const auto *method = classType->getMethod(memberExpr->member);
      if (method) {
        return method->returnType;
      }
    }

    error("Member not found: " + memberExpr->member, *memberExpr);
    return nullptr;
  }

  error("Object is not a class type", *memberExpr);
  return nullptr;
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
  return nullptr;
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
  // 将 AST PrimitiveType::Kind 转换为 types PrimitiveType::Kind
  switch (primitiveType->kind) {
  case ast::PrimitiveType::Kind::Void:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  case ast::PrimitiveType::Kind::Bool:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Bool);
  case ast::PrimitiveType::Kind::Byte:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Byte);
  case ast::PrimitiveType::Kind::SByte:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::SByte);
  case ast::PrimitiveType::Kind::Short:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Short);
  case ast::PrimitiveType::Kind::UShort:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::UShort);
  case ast::PrimitiveType::Kind::Int:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Int);
  case ast::PrimitiveType::Kind::UInt:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::UInt);
  case ast::PrimitiveType::Kind::Long:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Long);
  case ast::PrimitiveType::Kind::ULong:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::ULong);
  case ast::PrimitiveType::Kind::Float:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Float);
  case ast::PrimitiveType::Kind::Double:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Double);
  case ast::PrimitiveType::Kind::Fp16:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Fp16);
  case ast::PrimitiveType::Kind::Bf16:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Bf16);
  case ast::PrimitiveType::Kind::Char:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Char);
  default:
    return nullptr;
  }
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
  return analyzeTypeByName(namedType->name);
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTupleType(const ast::TupleType *tupleType) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTypeByName(const std::string &typeName) {
  // 首先检查是否是基本类型
  auto primitiveType = types::TypeFactory::getPrimitiveTypeByName(typeName);
  if (primitiveType) {
    return primitiveType;
  }

  // 检查符号表
  auto symbol = symbolTable.lookupSymbol(typeName);
  if (auto typeAliasSymbol =
          std::dynamic_pointer_cast<TypeAliasSymbol>(symbol)) {
    return typeAliasSymbol->getType();
  }
  if (auto classSymbol = std::dynamic_pointer_cast<ClassSymbol>(symbol)) {
    return classSymbol->getType();
  }
  if (auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol)) {
    return structSymbol->getType();
  }

  return nullptr;
}

// 将 Visibility 转换为 AccessModifier
types::AccessModifier
SemanticAnalyzer::visibilityToAccessModifier(Visibility visibility) {
  switch (visibility) {
  case Visibility::Public:
    return types::AccessModifier::Public;
  case Visibility::Private:
    return types::AccessModifier::Private;
  case Visibility::Protected:
    return types::AccessModifier::Protected;
  default:
    return types::AccessModifier::Public; // 默认公共访问
  }
}

bool SemanticAnalyzer::isLValue(const ast::Expression &expr) const {
  return false;
}
bool SemanticAnalyzer::isTypeCompatible(
    const std::shared_ptr<types::Type> &expected,
    const std::shared_ptr<types::Type> &actual) {
  // 精确匹配
  if (expected->toString() == actual->toString()) {
    return true;
  }

  // 检查子类型关系
  if (actual->isSubtypeOf(*expected)) {
    return true;
  }

  // 检查隐式转换
  if (tryImplicitConversion(expected, actual)) {
    return true;
  }

  return false;
}
std::shared_ptr<types::Type> SemanticAnalyzer::tryImplicitConversion(
    const std::shared_ptr<types::Type> &expected,
    const std::shared_ptr<types::Type> &actual) {
  // 检查数字类型之间的隐式转换
  // 例如：int -> long, float -> double 等
  if (auto expectedPrimitive =
          std::dynamic_pointer_cast<types::PrimitiveType>(expected)) {
    if (auto actualPrimitive =
            std::dynamic_pointer_cast<types::PrimitiveType>(actual)) {
      // 数字类型转换规则
      // 小整数类型可以转换为大整数类型
      // 整数可以转换为浮点数
      // 浮点数可以转换为更大的浮点数
      switch (actualPrimitive->getKind()) {
      case types::PrimitiveType::Kind::Int:
        // int 可以转换为 long, float, double
        switch (expectedPrimitive->getKind()) {
        case types::PrimitiveType::Kind::Long:
        case types::PrimitiveType::Kind::Float:
        case types::PrimitiveType::Kind::Double:
          return expected;
        default:
          break;
        }
        break;
      case types::PrimitiveType::Kind::Long:
        // long 可以转换为 float, double
        switch (expectedPrimitive->getKind()) {
        case types::PrimitiveType::Kind::Float:
        case types::PrimitiveType::Kind::Double:
          return expected;
        default:
          break;
        }
        break;
      case types::PrimitiveType::Kind::Float:
        // float 可以转换为 double
        if (expectedPrimitive->getKind() ==
            types::PrimitiveType::Kind::Double) {
          return expected;
        }
        break;
      default:
        break;
      }
    }
  }

  // 检查指针和切片之间的转换
  // 例如：LiteralView 到 byte![] 或 byte!^ 的转换
  // 这里需要根据实际的类型系统实现来添加相应的转换规则

  return nullptr;
}
Visibility
SemanticAnalyzer::parseVisibility(const std::vector<std::string> &specifiers) {
  return Visibility::Public;
}

void SemanticAnalyzer::error(const std::string &message,
                             const ast::Node &node) {
  hasError_ = true;
  std::cerr << "Semantic Error: " << message << std::endl;
}

} // namespace semantic
} // namespace c_hat