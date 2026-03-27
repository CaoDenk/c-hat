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

  // 第一遍：收集所有顶级声明，创建它们的符号并添加到全局符号表中
  for (auto &decl : program.declarations) {
    if (auto *classDecl = dynamic_cast<ast::ClassDecl *>(decl.get())) {
      // 创建类类型
      auto classType = std::make_shared<types::ClassType>(classDecl->name);

      // 创建类符号并添加到符号表
      auto classSymbol =
          std::make_shared<ClassSymbol>(classDecl->name, classType);
      symbolTable.addSymbol(classSymbol);
    } else if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(decl.get())) {
      // 处理函数声明
      analyzeFunctionDecl(funcDecl);
    } else if (auto *interfaceDecl =
                   dynamic_cast<ast::InterfaceDecl *>(decl.get())) {
      analyzeInterfaceDecl(interfaceDecl);
    } else if (auto *structDecl = dynamic_cast<ast::StructDecl *>(decl.get())) {
      analyzeStructDecl(structDecl);
    } else if (auto *enumDecl = dynamic_cast<ast::EnumDecl *>(decl.get())) {
      analyzeEnumDecl(enumDecl);
    } else if (auto *typeAliasDecl =
                   dynamic_cast<ast::TypeAliasDecl *>(decl.get())) {
      analyzeTypeAliasDecl(typeAliasDecl);
    } else if (auto *moduleDecl = dynamic_cast<ast::ModuleDecl *>(decl.get())) {
      analyzeModuleDecl(moduleDecl);
    } else if (auto *importDecl = dynamic_cast<ast::ImportDecl *>(decl.get())) {
      analyzeImportDecl(importDecl);
    }
  }

  // 第二遍：分析类成员（使用 analyzeClassDecl 方法）
  for (auto &decl : program.declarations) {
    if (auto *classDecl = dynamic_cast<ast::ClassDecl *>(decl.get())) {
      // 分析类声明，包括类成员
      analyzeClassDecl(classDecl);
    }
  }

  // 第三遍：分析函数体和属性方法体
  for (auto &decl : program.declarations) {
    if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(decl.get())) {
      // 分析函数体
      if (funcDecl->body) {
        if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
          analyzeStatement(stmt);
        }
      }
    }
    // 注意：类的方法体和属性方法体已经在 analyzeClassDecl 中分析过了
    // 所以这里不需要再次处理类声明
  }

  // 检查是否存在 main 函数
  bool hasMainFunction = false;
  auto mainSymbol = symbolTable.lookupSymbol("main");
  if (mainSymbol) {
    if (auto *funcSymbol = dynamic_cast<FunctionSymbol *>(mainSymbol.get())) {
      hasMainFunction = true;
    }
  }

  if (!hasMainFunction) {
    error("No main function found");
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

void SemanticAnalyzer::analyzeVariableDecl(ast::VariableDecl *varDecl) {
  // 分析变量类型
  std::shared_ptr<types::Type> varType;
  if (varDecl->type) {
    if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
      varType = analyzeType(typeNode);
      if (!varType) {
        error("Invalid variable type", *varDecl);
      }
    }
  }

  // 分析初始化表达式
  if (varDecl->initializer) {
    auto initType = analyzeExpression(varDecl->initializer.get());
    // 如果变量没有显式类型，则从初始化表达式推断类型
    if (!varType && initType) {
      varType = initType;
    }
  }

  // 将变量添加到符号表
  auto variableSymbol = std::make_shared<VariableSymbol>(
      varDecl->name, varType, varDecl->kind, varDecl->isConst);
  symbolTable.addSymbol(variableSymbol);
}
void SemanticAnalyzer::analyzeTupleDestructuringDecl(
    ast::TupleDestructuringDecl *decl) {}

void SemanticAnalyzer::analyzeFunctionDecl(
    ast::FunctionDecl *funcDecl,
    std::shared_ptr<types::ClassType> currentClassType) {
  // 分析模板参数
  std::vector<std::shared_ptr<types::Type>> templateParams;
  if (!funcDecl->templateParams.empty()) {
    templateParams = analyzeTemplateParameters(funcDecl->templateParams);
  }

  // 分析参数
  std::vector<std::shared_ptr<types::Type>> paramTypes;
  for (auto &param : funcDecl->params) {
    if (auto *paramNode = dynamic_cast<ast::Parameter *>(param.get())) {
      // 处理 self 参数
      if (paramNode->isSelf) {
        // self 参数的类型是当前类的类型
        if (currentClassType) {
          paramTypes.push_back(currentClassType);
        }
        continue;
      }
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
  auto funcSymbol = std::make_shared<FunctionSymbol>(funcDecl->name, funcType, Visibility::Default, funcDecl->isImmutable);
  symbolTable.addSymbol(funcSymbol);

  // 进入函数作用域
  symbolTable.enterScope();

  // 添加参数到函数作用域
  for (auto &param : funcDecl->params) {
    if (auto *paramNode = dynamic_cast<ast::Parameter *>(param.get())) {
      // 处理 self 参数
      if (paramNode->isSelf) {
        if (currentClassType) {
          auto selfSymbol = std::make_shared<VariableSymbol>(paramNode->name, currentClassType);
          symbolTable.addSymbol(selfSymbol);
        }
        continue;
      }
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
      analyzeStatement(stmt, currentClassType);
    }
  }

  // 退出函数作用域
  symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeClassDecl(ast::ClassDecl *classDecl) {
  // 保存当前的类名
  std::string oldClassName = currentClassName_;
  currentClassName_ = classDecl->name;

  // 分析模板参数
  std::vector<std::string> typeParameters;
  if (!classDecl->templateParams.empty()) {
    for (const auto &param : classDecl->templateParams) {
      if (auto *templateParam =
              dynamic_cast<ast::TemplateParameter *>(param.get())) {
        typeParameters.push_back(templateParam->name);
      }
    }
  }

  // 获取在第一遍中创建的类类型
  auto classSymbol = std::dynamic_pointer_cast<ClassSymbol>(
      symbolTable.lookupSymbol(classDecl->name));
  if (!classSymbol) {
    error("Class symbol not found: " + classDecl->name, *classDecl);
    return;
  }
  auto classType =
      std::dynamic_pointer_cast<types::ClassType>(classSymbol->getType());
  if (!classType) {
    error("Class type not found: " + classDecl->name, *classDecl);
    return;
  }

  // 处理基类（单继承和多继承）
  if (!classDecl->baseClass.empty()) {
    std::cerr << "Debug: Processing base class: " << classDecl->baseClass
              << std::endl;
    auto baseType = analyzeTypeByName(classDecl->baseClass);
    if (baseType) {
      std::cerr << "Debug: baseType found, isClass: " << baseType->isClass()
                << std::endl;
      if (baseType->isClass()) {
        auto baseClassType =
            std::dynamic_pointer_cast<types::ClassType>(baseType);

        // 检查循环继承
        if (checkCircularInheritance(classType.get(), baseClassType.get())) {
          error("Circular inheritance detected: " + classDecl->name +
                    " inherits from " + classDecl->baseClass,
                *classDecl);
        } else {
          classType->addBaseClass(baseClassType);
          std::cerr << "Debug: Added base class: " << classDecl->baseClass
                    << std::endl;
        }
      } else {
        error("Base class not found: " + classDecl->baseClass, *classDecl);
      }
    } else {
      std::cerr << "Debug: baseType is null" << std::endl;
      error("Base class not found: " + classDecl->baseClass, *classDecl);
    }
  }

  // 处理多继承
  for (const auto &baseClassName : classDecl->baseClasses) {
    if (baseClassName == classDecl->baseClass) {
      continue; // 跳过第一个基类，已经处理过了
    }

    auto baseType = analyzeTypeByName(baseClassName);
    if (baseType && baseType->isClass()) {
      auto baseClassType =
          std::dynamic_pointer_cast<types::ClassType>(baseType);

      // 检查循环继承
      if (checkCircularInheritance(classType.get(), baseClassType.get())) {
        error("Circular inheritance detected: " + classDecl->name +
                  " inherits from " + baseClassName,
              *classDecl);
      } else {
        classType->addBaseClass(baseClassType);
      }
    } else {
      error("Base class not found: " + baseClassName, *classDecl);
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

  // 检查类是否实现了所有接口方法
  checkInterfaceImplementation(classDecl, classType.get());

  // 进入类作用域
  symbolTable.enterScope();

  // 首先分析所有变量声明，将它们添加到符号表中
  for (auto &member : classDecl->members) {
    if (auto *varDecl = dynamic_cast<ast::VariableDecl *>(member.get())) {
      // 分析变量类型
      std::shared_ptr<types::Type> varType;
      if (varDecl->type) {
        if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
          varType = analyzeType(typeNode);
          if (!varType) {
            error("Invalid variable type", *varDecl);
          }
        }
      }
      // 将变量添加到符号表
      auto variableSymbol =
          std::make_shared<VariableSymbol>(varDecl->name, varType);
      symbolTable.addSymbol(variableSymbol);
    }
  }

  // 添加基类的成员变量到符号表中
  for (const auto &baseClass : classType->getBaseClasses()) {
    for (const auto &fieldPair : baseClass->getFields()) {
      const auto &field = fieldPair.second;
      // 只添加 protected 和 public 的成员变量
      if (field.access == types::AccessModifier::Protected ||
          field.access == types::AccessModifier::Public) {
        auto variableSymbol =
            std::make_shared<VariableSymbol>(field.name, field.type);
        symbolTable.addSymbol(variableSymbol);
      }
    }
  }

  // 然后分析所有成员
  for (auto &member : classDecl->members) {
    if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(member.get())) {
      // 分析方法声明
      analyzeFunctionDecl(funcDecl, classType);

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
      std::cerr << "Debug: Added field " << varDecl->name << " to class "
                << classDecl->name << " with access "
                << static_cast<int>(access) << std::endl;
    } else if (auto *getterDecl =
                   dynamic_cast<ast::GetterDecl *>(member.get())) {
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
          continue; // 跳过创建属性
        }
      } else {
        error("Invalid property type", *getterDecl);
        continue; // 跳过创建属性
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

      // 分析 getter 方法体
      if (getterDecl->body) {
        if (auto *compoundStmt =
                dynamic_cast<ast::CompoundStmt *>(getterDecl->body.get())) {
          analyzeStatement(compoundStmt);
        }
      }

      // 分析 getter 箭头表达式（如果有）
      if (getterDecl->arrowExpr) {
        analyzeExpression(getterDecl->arrowExpr.get());
      }
    } else if (auto *setterDecl =
                   dynamic_cast<ast::SetterDecl *>(member.get())) {
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
            continue; // 跳过创建属性
          }
        } else {
          error("Invalid property type", *setterDecl);
          continue; // 跳过创建属性
        }
      } else {
        error("Invalid property parameter", *setterDecl);
        continue; // 跳过创建属性
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

      // 分析 setter 方法体
      if (setterDecl->body) {
        if (auto *compoundStmt =
                dynamic_cast<ast::CompoundStmt *>(setterDecl->body.get())) {
          analyzeStatement(compoundStmt);
        }
      }

      // 分析 setter 箭头表达式（如果有）
      if (setterDecl->arrowExpr) {
        analyzeExpression(setterDecl->arrowExpr.get());
      }
    } else if (auto *decl = dynamic_cast<ast::Declaration *>(member.get())) {
      analyzeDeclaration(decl);
    } else if (auto *stmt = dynamic_cast<ast::Statement *>(member.get())) {
      analyzeStatement(stmt);
    }
  }

  // 退出类作用域
  symbolTable.exitScope();

  // 恢复当前的类名
  currentClassName_ = oldClassName;
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
      // 将参数添加到作用域
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

  // 移除参数符号，避免作用域污染
  if (setterDecl->param) {
    if (auto *paramNode =
            dynamic_cast<ast::Parameter *>(setterDecl->param.get())) {
      // 注意：这里简化处理，实际应该从当前作用域中移除符号
      // 由于 SymbolTable 目前没有提供移除符号的方法，我们暂时不做处理
      // 但在实际实现中，应该添加这个功能
    }
  }
}
void SemanticAnalyzer::analyzeExternDecl(ast::ExternDecl *externDecl) {}
void SemanticAnalyzer::analyzeNamespaceDecl(ast::NamespaceDecl *namespaceDecl) {
}

std::shared_ptr<types::Type> SemanticAnalyzer::analyzeStatement(
    ast::Statement *stmt, std::shared_ptr<types::ClassType> currentClassType) {
  switch (stmt->getType()) {
  case ast::NodeType::ExprStmt:
    return analyzeExprStmt(static_cast<ast::ExprStmt *>(stmt));
  case ast::NodeType::CompoundStmt:
    return analyzeCompoundStmt(static_cast<ast::CompoundStmt *>(stmt),
                               currentClassType);
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

std::shared_ptr<types::Type> SemanticAnalyzer::analyzeCompoundStmt(
    ast::CompoundStmt *compoundStmt,
    std::shared_ptr<types::ClassType> currentClassType) {
  symbolTable.enterScope();

  // 如果当前在类作用域中（即当前作用域级别 >
  // 1），则将类成员变量复制到当前作用域中 这样就可以在方法体中访问类成员变量
  if (symbolTable.getCurrentScopeLevel() > 1) {
    auto parentSymbols = symbolTable.getSymbolsInParentScope();
    for (const auto &symbol : parentSymbols) {
      // 只复制变量符号，不复制类符号、函数符号等
      if (auto variableSymbol =
              std::dynamic_pointer_cast<VariableSymbol>(symbol)) {
        symbolTable.addSymbol(variableSymbol);
      }
    }

    // 如果当前在类作用域中，复制基类的成员变量
    if (currentClassType) {
      std::cerr << "Debug: currentClassType found" << std::endl;
      std::cerr << "Debug: base class count = "
                << currentClassType->getBaseClasses().size() << std::endl;
      // 获取基类的成员变量
      for (const auto &baseClass : currentClassType->getBaseClasses()) {
        std::cerr << "Debug: baseClass = " << baseClass->getName() << std::endl;
        for (const auto &fieldPair : baseClass->getFields()) {
          const auto &field = fieldPair.second;
          std::cerr << "Debug: field = " << field.name << std::endl;
          // 只复制 protected 和 public 的成员变量
          if (field.access == types::AccessModifier::Protected ||
              field.access == types::AccessModifier::Public) {
            auto variableSymbol =
                std::make_shared<VariableSymbol>(field.name, field.type);
            symbolTable.addSymbol(variableSymbol);
            std::cerr << "Debug: added variable " << field.name
                      << " to symbol table" << std::endl;
          }
        }
      }
    } else if (!currentClassName_.empty()) {
      std::cerr << "Debug: currentClassName_ = " << currentClassName_
                << std::endl;
      auto classSymbol = std::dynamic_pointer_cast<ClassSymbol>(
          symbolTable.lookupSymbol(currentClassName_));
      if (classSymbol) {
        std::cerr << "Debug: classSymbol found" << std::endl;
        auto classType =
            std::dynamic_pointer_cast<types::ClassType>(classSymbol->getType());
        if (classType) {
          std::cerr << "Debug: classType found" << std::endl;
          std::cerr << "Debug: base class count = "
                    << classType->getBaseClasses().size() << std::endl;
          // 获取基类的成员变量
          for (const auto &baseClass : classType->getBaseClasses()) {
            std::cerr << "Debug: baseClass = " << baseClass->getName()
                      << std::endl;
            for (const auto &fieldPair : baseClass->getFields()) {
              const auto &field = fieldPair.second;
              std::cerr << "Debug: field = " << field.name << std::endl;
              // 只复制 protected 和 public 的成员变量
              if (field.access == types::AccessModifier::Protected ||
                  field.access == types::AccessModifier::Public) {
                auto variableSymbol =
                    std::make_shared<VariableSymbol>(field.name, field.type);
                symbolTable.addSymbol(variableSymbol);
                std::cerr << "Debug: added variable " << field.name
                          << " to symbol table" << std::endl;
              }
            }
          }
        } else {
          std::cerr << "Debug: classType is null" << std::endl;
        }
      } else {
        std::cerr << "Debug: classSymbol is null" << std::endl;
      }
    }
  }

  std::shared_ptr<types::Type> lastType;
  for (auto &stmt : compoundStmt->statements) {
    // 先检查是否是 VariableStmt
    if (auto *varStmt = dynamic_cast<ast::VariableStmt *>(stmt.get())) {
      analyzeVariableDecl(varStmt->declaration.get());
    } else if (auto *statement = dynamic_cast<ast::Statement *>(stmt.get())) {
      lastType = analyzeStatement(statement, currentClassType);
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
  // 分析 match 的表达式
  if (matchStmt->expr) {
    analyzeExpression(matchStmt->expr.get());
  }

  // 分析每个 match arm
  for (auto &arm : matchStmt->arms) {
    if (arm) {
      analyzeStatement(arm->body.get());
    }
  }

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
  case ast::NodeType::ArrayInitExpr:
    return analyzeArrayInitExpr(static_cast<ast::ArrayInitExpr *>(expression));
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

    // 检查是否是 let 变量的重新赋值
    if (auto *identifier =
            dynamic_cast<ast::Identifier *>(binaryExpr->left.get())) {
      auto symbol = symbolTable.lookupSymbol(identifier->name);
      if (symbol) {
        if (auto *varSymbol = dynamic_cast<VariableSymbol *>(symbol.get())) {
          if (varSymbol->isImmutableBinding()) {
            error("Cannot reassign let variable: " + identifier->name,
                  *binaryExpr);
            return nullptr;
          }
        }
      }
    }

    // 检查是否是 const 数组元素的修改
    if (auto *subscriptExpr =
            dynamic_cast<ast::SubscriptExpr *>(binaryExpr->left.get())) {
      if (auto *identifier =
              dynamic_cast<ast::Identifier *>(subscriptExpr->object.get())) {
        auto symbol = symbolTable.lookupSymbol(identifier->name);
        if (symbol) {
          if (auto *varSymbol = dynamic_cast<VariableSymbol *>(symbol.get())) {
            if (varSymbol->isConst()) {
              error("Cannot modify elements of const array: " +
                        identifier->name,
                    *binaryExpr);
              return nullptr;
            }
          }
        }
      }
    }

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
  // 在符号表中查找标识符
  auto symbol = symbolTable.lookupSymbol(identifier->name);
  if (symbol) {
    // 检查符号类型
    if (auto variableSymbol =
            std::dynamic_pointer_cast<VariableSymbol>(symbol)) {
      return variableSymbol->getType();
    } else if (auto classSymbol =
                   std::dynamic_pointer_cast<ClassSymbol>(symbol)) {
      return classSymbol->getType();
    } else if (auto functionSymbol =
                   std::dynamic_pointer_cast<FunctionSymbol>(symbol)) {
      return functionSymbol->getType();
    }
  }

  // 未找到标识符
  error("Undeclared identifier: " + identifier->name, *identifier);
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

  // 检查是否是数组类型
  if (objectType->isArray()) {
    auto arrayType = std::dynamic_pointer_cast<types::ArrayType>(objectType);

    // 数组的 len 属性
    if (memberExpr->member == "len") {
      return types::TypeFactory::getPrimitiveType(
          types::PrimitiveType::Kind::Int);
    }

    // 数组的 ptr 属性
    if (memberExpr->member == "ptr") {
      auto elementType = arrayType->getElementType();
      return std::make_shared<types::PointerType>(elementType);
    }

    error("Member not found in array: " + memberExpr->member, *memberExpr);
    return nullptr;
  }

  // 检查是否是切片类型
  if (objectType->isSlice()) {
    auto sliceType = std::dynamic_pointer_cast<types::SliceType>(objectType);

    // 切片的 len 属性
    if (memberExpr->member == "len") {
      return types::TypeFactory::getPrimitiveType(
          types::PrimitiveType::Kind::Int);
    }

    // 切片的 ptr 属性
    if (memberExpr->member == "ptr") {
      auto elementType = sliceType->getElementType();
      return std::make_shared<types::PointerType>(elementType);
    }

    error("Member not found in slice: " + memberExpr->member, *memberExpr);
    return nullptr;
  }

  // 检查是否是类类型
  if (objectType->isClass()) {
    auto classType = std::dynamic_pointer_cast<types::ClassType>(objectType);

    // 检查字段是否存在
    if (classType->hasField(memberExpr->member)) {
      const auto *field = classType->getField(memberExpr->member);
      if (field) {
        // 检查访问权限
        if (!checkAccessControl(field->access, classType.get())) {
          error("Access denied to field: " + memberExpr->member, *memberExpr);
          return nullptr;
        }
        return field->type;
      }
    }

    // 检查属性是否存在
    if (classType->hasProperty(memberExpr->member)) {
      const auto *property = classType->getProperty(memberExpr->member);
      if (property && property->hasGetter) {
        // 检查访问权限
        if (!checkAccessControl(property->access, classType.get())) {
          error("Access denied to property: " + memberExpr->member,
                *memberExpr);
          return nullptr;
        }
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
        // 检查访问权限
        if (!checkAccessControl(method->access, classType.get())) {
          error("Access denied to method: " + memberExpr->member, *memberExpr);
          return nullptr;
        }
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
  // 分析数组对象
  auto arrayType = analyzeExpression(subscriptExpr->object.get());
  if (!arrayType) {
    error("Invalid array type in subscript", *subscriptExpr);
    return nullptr;
  }

  // 分析下标表达式
  auto indexType = analyzeExpression(subscriptExpr->index.get());
  if (!indexType) {
    error("Invalid index type in subscript", *subscriptExpr);
    return nullptr;
  }

  // 检查是否是数组类型
  if (arrayType->isArray()) {
    auto array = std::dynamic_pointer_cast<types::ArrayType>(arrayType);
    return array->getElementType();
  }

  // 检查是否是切片类型
  if (arrayType->isSlice()) {
    auto slice = std::dynamic_pointer_cast<types::SliceType>(arrayType);
    return slice->getElementType();
  }

  // 检查是否是指针类型
  if (arrayType->isPointer()) {
    auto pointer = std::dynamic_pointer_cast<types::PointerType>(arrayType);
    return pointer->getPointeeType();
  }

  error("Subscript can only be used with arrays, slices, or pointers",
        *subscriptExpr);
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
  // 分析数组元素类型
  std::shared_ptr<types::Type> elementType;

  for (auto &element : arrayInitExpr->elements) {
    auto elementExprType = analyzeExpression(element.get());
    if (!elementExprType) {
      return nullptr;
    }

    // 如果还没有确定元素类型，则使用第一个元素的类型
    if (!elementType) {
      elementType = elementExprType;
    } else {
      // 检查所有元素的类型是否一致
      if (!isTypeCompatible(elementType, elementExprType)) {
        error("Array elements must have the same type", *arrayInitExpr);
        return nullptr;
      }
    }
  }

  // 如果没有元素，则默认使用 int 类型
  if (!elementType) {
    elementType =
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int);
  }

  // 创建数组类型
  return std::make_shared<types::ArrayType>(elementType,
                                            arrayInitExpr->elements.size());
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
  // 分析元素类型
  std::shared_ptr<types::Type> elementType;
  if (pointerType->baseType) {
    if (auto *typeNode =
            dynamic_cast<const ast::Type *>(pointerType->baseType.get())) {
      elementType = analyzeType(typeNode);
    }
  }

  if (!elementType) {
    return nullptr;
  }

  // 创建指针类型
  return std::make_shared<types::PointerType>(elementType);
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeArrayType(const ast::ArrayType *arrayType) {
  // 分析元素类型
  std::shared_ptr<types::Type> elementType;
  if (arrayType->baseType) {
    if (auto *typeNode =
            dynamic_cast<const ast::Type *>(arrayType->baseType.get())) {
      elementType = analyzeType(typeNode);
    }
  }

  if (!elementType) {
    return nullptr;
  }

  // 获取数组大小（需要计算常量表达式）
  size_t arraySize = 0;
  if (arrayType->size) {
    // 尝试从字面量获取大小
    if (auto *literal =
            dynamic_cast<const ast::Literal *>(arrayType->size.get())) {
      if (literal->type == ast::Literal::Type::Integer) {
        arraySize = std::stoull(literal->value);
      }
    }
  }

  // 创建数组类型
  return std::make_shared<types::ArrayType>(elementType, arraySize);
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSliceType(const ast::SliceType *sliceType) {
  // 分析元素类型
  std::shared_ptr<types::Type> elementType;
  if (sliceType->baseType) {
    if (auto *typeNode =
            dynamic_cast<const ast::Type *>(sliceType->baseType.get())) {
      elementType = analyzeType(typeNode);
    }
  }

  if (!elementType) {
    return nullptr;
  }

  // 创建切片类型
  return std::make_shared<types::SliceType>(elementType);
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

  // 特殊处理 string 类型
  if (typeName == "string") {
    return std::make_shared<types::ClassType>("string");
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
  // 处理空类型
  if (!expected || !actual) {
    return false;
  }

  // 精确匹配
  if (expected->toString() == actual->toString()) {
    return true;
  }

  // 检查数组类型的大小匹配
  if (expected->isArray() && actual->isArray()) {
    auto expectedArray = std::dynamic_pointer_cast<types::ArrayType>(expected);
    auto actualArray = std::dynamic_pointer_cast<types::ArrayType>(actual);

    // 检查元素类型是否相同
    if (!isTypeCompatible(expectedArray->getElementType(),
                          actualArray->getElementType())) {
      return false;
    }

    // 检查数组大小是否相同
    if (expectedArray->getSize() != actualArray->getSize()) {
      return false;
    }

    return true;
  }

  // 检查数组到切片的转换
  if (expected->isSlice() && actual->isArray()) {
    auto expectedSlice = std::dynamic_pointer_cast<types::SliceType>(expected);
    auto actualArray = std::dynamic_pointer_cast<types::ArrayType>(actual);

    // 检查元素类型是否兼容
    return isTypeCompatible(expectedSlice->getElementType(),
                            actualArray->getElementType());
  }

  // 检查切片类型兼容性
  if (expected->isSlice() && actual->isSlice()) {
    auto expectedSlice = std::dynamic_pointer_cast<types::SliceType>(expected);
    auto actualSlice = std::dynamic_pointer_cast<types::SliceType>(actual);

    // 检查元素类型是否兼容
    return isTypeCompatible(expectedSlice->getElementType(),
                            actualSlice->getElementType());
  }

  // 检查指针类型兼容性
  if (expected->isPointer() && actual->isPointer()) {
    auto expectedPtr = std::dynamic_pointer_cast<types::PointerType>(expected);
    auto actualPtr = std::dynamic_pointer_cast<types::PointerType>(actual);

    // 检查指向类型是否兼容
    return isTypeCompatible(expectedPtr->getPointeeType(),
                            actualPtr->getPointeeType());
  }

  // 检查指针和数组之间的转换
  if (expected->isPointer() && actual->isArray()) {
    auto expectedPtr = std::dynamic_pointer_cast<types::PointerType>(expected);
    auto actualArray = std::dynamic_pointer_cast<types::ArrayType>(actual);

    // 数组可以隐式转换为指向其元素类型的指针
    return isTypeCompatible(expectedPtr->getPointeeType(),
                            actualArray->getElementType());
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
  // 处理空类型
  if (!expected || !actual) {
    return nullptr;
  }

  // 检查数字类型之间的隐式转换
  if (auto expectedPrimitive =
          std::dynamic_pointer_cast<types::PrimitiveType>(expected)) {
    if (auto actualPrimitive =
            std::dynamic_pointer_cast<types::PrimitiveType>(actual)) {
      auto expectedKind = expectedPrimitive->getKind();
      auto actualKind = actualPrimitive->getKind();

      // 相同类型无需转换
      if (expectedKind == actualKind) {
        return expected;
      }

      // 整数类型之间的转换规则
      // 小整数类型可以安全转换为大整数类型
      switch (actualKind) {
      // 8位整数
      case types::PrimitiveType::Kind::Byte:
      case types::PrimitiveType::Kind::SByte:
        // 可以转换为任何更大的整数类型或浮点数类型
        switch (expectedKind) {
        case types::PrimitiveType::Kind::Short:
        case types::PrimitiveType::Kind::UShort:
        case types::PrimitiveType::Kind::Int:
        case types::PrimitiveType::Kind::UInt:
        case types::PrimitiveType::Kind::Long:
        case types::PrimitiveType::Kind::ULong:
        case types::PrimitiveType::Kind::Float:
        case types::PrimitiveType::Kind::Double:
          return expected;
        default:
          break;
        }
        break;

      // 16位整数
      case types::PrimitiveType::Kind::Short:
      case types::PrimitiveType::Kind::UShort:
        // 可以转换为更大的整数类型或浮点数类型
        switch (expectedKind) {
        case types::PrimitiveType::Kind::Int:
        case types::PrimitiveType::Kind::UInt:
        case types::PrimitiveType::Kind::Long:
        case types::PrimitiveType::Kind::ULong:
        case types::PrimitiveType::Kind::Float:
        case types::PrimitiveType::Kind::Double:
          return expected;
        default:
          break;
        }
        break;

      // 32位整数
      case types::PrimitiveType::Kind::Int:
      case types::PrimitiveType::Kind::UInt:
        // 可以转换为64位整数或浮点数类型
        switch (expectedKind) {
        case types::PrimitiveType::Kind::Long:
        case types::PrimitiveType::Kind::ULong:
        case types::PrimitiveType::Kind::Float:
        case types::PrimitiveType::Kind::Double:
          return expected;
        default:
          break;
        }
        break;

      // 64位整数
      case types::PrimitiveType::Kind::Long:
      case types::PrimitiveType::Kind::ULong:
        // 可以转换为浮点数类型（可能丢失精度）
        switch (expectedKind) {
        case types::PrimitiveType::Kind::Float:
        case types::PrimitiveType::Kind::Double:
          return expected;
        default:
          break;
        }
        break;

      // 浮点数类型
      case types::PrimitiveType::Kind::Float:
        // float 可以转换为 double
        if (expectedKind == types::PrimitiveType::Kind::Double) {
          return expected;
        }
        break;

      // 字符类型可以转换为整数类型
      case types::PrimitiveType::Kind::Char:
        switch (expectedKind) {
        case types::PrimitiveType::Kind::Int:
        case types::PrimitiveType::Kind::UInt:
        case types::PrimitiveType::Kind::Long:
        case types::PrimitiveType::Kind::ULong:
          return expected;
        default:
          break;
        }
        break;

      default:
        break;
      }
    }
  }

  // 检查指针和数组之间的转换
  // 数组可以隐式转换为指向其元素类型的指针
  if (expected->isPointer() && actual->isArray()) {
    auto expectedPtr = std::dynamic_pointer_cast<types::PointerType>(expected);
    auto actualArray = std::dynamic_pointer_cast<types::ArrayType>(actual);

    if (isTypeCompatible(expectedPtr->getPointeeType(),
                         actualArray->getElementType())) {
      return expected;
    }
  }

  // 检查数组到切片的转换
  if (expected->isSlice() && actual->isArray()) {
    auto expectedSlice = std::dynamic_pointer_cast<types::SliceType>(expected);
    auto actualArray = std::dynamic_pointer_cast<types::ArrayType>(actual);

    if (isTypeCompatible(expectedSlice->getElementType(),
                         actualArray->getElementType())) {
      return expected;
    }
  }

  // 检查null指针转换
  // null可以转换为任何指针类型
  if (expected->isPointer()) {
    // 检查actual是否是null类型
    if (auto actualPrimitive =
            std::dynamic_pointer_cast<types::PrimitiveType>(actual)) {
      if (actualPrimitive->getKind() == types::PrimitiveType::Kind::Void) {
        // 假设void类型表示null
        return expected;
      }
    }
  }

  return nullptr;
}
Visibility
SemanticAnalyzer::parseVisibility(const std::vector<std::string> &specifiers) {
  for (const auto &specifier : specifiers) {
    if (specifier == "public") {
      return Visibility::Public;
    } else if (specifier == "private") {
      return Visibility::Private;
    } else if (specifier == "protected") {
      return Visibility::Protected;
    } else if (specifier == "internal") {
      return Visibility::Internal;
    }
  }
  return Visibility::Default;
}

void SemanticAnalyzer::error(const std::string &message,
                             const ast::Node &node) {
  hasError_ = true;
  std::cerr << "Semantic Error: " << message << std::endl;
}

void SemanticAnalyzer::error(const std::string &message) {
  hasError_ = true;
  std::cerr << "Semantic Error: " << message << std::endl;
}

// 检查访问控制
bool SemanticAnalyzer::checkAccessControl(
    types::AccessModifier access, const types::ClassType *currentClass) {
  // 对于公共成员，总是可以访问
  if (access == types::AccessModifier::Public) {
    return true;
  }

  // 对于私有成员，只有当前类内部可以访问
  if (access == types::AccessModifier::Private) {
    // 这里简化处理，实际应该检查当前作用域是否在类内部
    // 暂时总是允许，因为我们还没有实现作用域跟踪
    return true;
  }

  // 对于受保护成员，当前类及其子类可以访问
  if (access == types::AccessModifier::Protected) {
    // 这里简化处理，实际应该检查访问者是否是子类
    // 暂时总是允许，因为我们还没有实现作用域跟踪
    return true;
  }

  return false;
}

// 检查循环继承
bool SemanticAnalyzer::checkCircularInheritance(const types::ClassType *derived,
                                                const types::ClassType *base) {
  // 检查基类是否继承自派生类（直接或间接）
  for (const auto &baseBase : base->getBaseClasses()) {
    if (baseBase->toString() == derived->toString()) {
      return true; // 发现循环继承
    }
    // 递归检查基类的基类
    if (checkCircularInheritance(derived, baseBase.get())) {
      return true;
    }
  }
  return false;
}

// 检查类是否实现了所有接口方法
void SemanticAnalyzer::checkInterfaceImplementation(
    ast::ClassDecl *classDecl, types::ClassType *classType) {
  for (const auto &interface : classType->getInterfaces()) {
    for (const auto &[methodName, interfaceMethod] : interface->getMethods()) {
      // 检查类是否实现了此方法
      if (!classType->hasMethod(methodName)) {
        error("Class " + classDecl->name +
                  " does not implement interface method " + methodName +
                  " from interface " + interface->toString(),
              *classDecl);
      } else {
        // 检查方法签名是否匹配
        const auto *classMethod = classType->getMethod(methodName);
        if (classMethod) {
          // 检查返回类型是否兼容
          if (!isTypeCompatible(interfaceMethod.returnType,
                                classMethod->returnType)) {
            error("Method " + methodName + " in class " + classDecl->name +
                      " has incompatible return type with interface " +
                      interface->toString(),
                  *classDecl);
          }
          // 检查参数类型是否匹配
          if (interfaceMethod.paramTypes.size() !=
              classMethod->paramTypes.size()) {
            error("Method " + methodName + " in class " + classDecl->name +
                      " has different number of parameters than interface " +
                      interface->toString(),
                  *classDecl);
          } else {
            for (size_t i = 0; i < interfaceMethod.paramTypes.size(); ++i) {
              if (!isTypeCompatible(interfaceMethod.paramTypes[i],
                                    classMethod->paramTypes[i])) {
                error("Method " + methodName + " in class " + classDecl->name +
                          " has incompatible parameter types with interface " +
                          interface->toString(),
                      *classDecl);
                break;
              }
            }
          }
        }
      }
    }
  }
}

// 分析模板参数
std::vector<std::shared_ptr<types::Type>>
SemanticAnalyzer::analyzeTemplateParameters(
    const std::vector<std::unique_ptr<ast::Node>> &params) {
  std::vector<std::shared_ptr<types::Type>> templateTypes;

  for (const auto &param : params) {
    if (auto *templateParam =
            dynamic_cast<ast::TemplateParameter *>(param.get())) {
      // 简化处理：创建一个泛型类型参数
      auto typeParam = std::make_shared<types::GenericType>(
          templateParam->name, std::vector<std::shared_ptr<types::Type>>());
      templateTypes.push_back(typeParam);
    }
  }

  return templateTypes;
}

// 分析模板实参
std::vector<std::shared_ptr<types::Type>>
SemanticAnalyzer::analyzeTemplateArguments(
    const std::vector<std::unique_ptr<ast::Node>> &args) {
  std::vector<std::shared_ptr<types::Type>> templateArgs;

  for (const auto &arg : args) {
    if (auto *typeNode = dynamic_cast<ast::Type *>(arg.get())) {
      auto type = analyzeType(typeNode);
      templateArgs.push_back(type);
    } else if (auto *exprNode = dynamic_cast<ast::Expression *>(arg.get())) {
      auto type = analyzeExpression(exprNode);
      templateArgs.push_back(type);
    }
  }

  return templateArgs;
}

// 实例化模板类型
std::shared_ptr<types::Type> SemanticAnalyzer::instantiateTemplateType(
    const std::shared_ptr<types::Type> &templateType,
    const std::vector<std::shared_ptr<types::Type>> &typeArguments) {
  if (templateType->isClass()) {
    auto classType = std::dynamic_pointer_cast<types::ClassType>(templateType);
    if (classType) {
      return classType->instantiate(typeArguments);
    }
  }

  // 对于其他类型，返回原始类型
  return templateType;
}

} // namespace semantic
} // namespace c_hat