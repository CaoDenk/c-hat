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
SemanticAnalyzer::SemanticAnalyzer(const std::string &stdlibPath) {
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
void SemanticAnalyzer::analyze(std::unique_ptr<ast::Program> program) {
  // 分析每个声明
  for (auto &declaration : program->declarations) {
    analyzeDeclaration(std::move(declaration));
  }
}

// 分析声明
void SemanticAnalyzer::analyzeDeclaration(
    std::unique_ptr<ast::Declaration> declaration) {
  switch (declaration->getType()) {
  case ast::NodeType::VariableDecl:
    analyzeVariableDecl(std::unique_ptr<ast::VariableDecl>(
        static_cast<ast::VariableDecl *>(declaration.release())));
    break;
  case ast::NodeType::FunctionDecl:
    analyzeFunctionDecl(std::unique_ptr<ast::FunctionDecl>(
        static_cast<ast::FunctionDecl *>(declaration.release())));
    break;
  case ast::NodeType::ClassDecl:
    analyzeClassDecl(std::unique_ptr<ast::ClassDecl>(
        static_cast<ast::ClassDecl *>(declaration.release())));
    break;
  case ast::NodeType::StructDecl:
    analyzeStructDecl(std::unique_ptr<ast::StructDecl>(
        static_cast<ast::StructDecl *>(declaration.release())));
    break;
  case ast::NodeType::EnumDecl:
    analyzeEnumDecl(std::unique_ptr<ast::EnumDecl>(
        static_cast<ast::EnumDecl *>(declaration.release())));
    break;
  case ast::NodeType::TypeAliasDecl:
    analyzeTypeAliasDecl(std::unique_ptr<ast::TypeAliasDecl>(
        static_cast<ast::TypeAliasDecl *>(declaration.release())));
    break;
  case ast::NodeType::ModuleDecl:
    analyzeModuleDecl(std::unique_ptr<ast::ModuleDecl>(
        static_cast<ast::ModuleDecl *>(declaration.release())));
    break;
  case ast::NodeType::ImportDecl:
    analyzeImportDecl(std::unique_ptr<ast::ImportDecl>(
        static_cast<ast::ImportDecl *>(declaration.release())));
    break;
  case ast::NodeType::ExtensionDecl:
    analyzeExtensionDecl(std::unique_ptr<ast::ExtensionDecl>(
        static_cast<ast::ExtensionDecl *>(declaration.release())));
    break;
  case ast::NodeType::GetterDecl:
    analyzeGetterDecl(std::unique_ptr<ast::GetterDecl>(
        static_cast<ast::GetterDecl *>(declaration.release())));
    break;
  case ast::NodeType::SetterDecl:
    analyzeSetterDecl(std::unique_ptr<ast::SetterDecl>(
        static_cast<ast::SetterDecl *>(declaration.release())));
    break;
  case ast::NodeType::ExternDecl:
    analyzeExternDecl(std::unique_ptr<ast::ExternDecl>(
        static_cast<ast::ExternDecl *>(declaration.release())));
    break;
  default:
    // 其他类型的声明，暂不处理
    break;
  }
}

// 分析结构体声明
void SemanticAnalyzer::analyzeStructDecl(
    std::unique_ptr<ast::StructDecl> structDecl) {
  // 检查当前作用域是否已存在该结构体
  if (symbolTable.hasSymbolInCurrentScope(structDecl->name)) {
    error(std::format("Struct '{}' already declared in current scope",
                      structDecl->name),
          *structDecl);
    return;
  }

  // 创建结构体类型
  auto structType = std::static_pointer_cast<types::ClassType>(
      types::TypeFactory::getClassType(structDecl->name));

  // 创建结构体符号
  Visibility visibility = parseVisibility(structDecl->specifiers);
  auto structSymbol =
      std::make_shared<StructSymbol>(structDecl->name, structType, visibility);

  // 添加到符号表
  symbolTable.addSymbol(structSymbol);

  // 分析结构体成员
  // 进入结构体作用域
  symbolTable.enterScope();

  // 分析结构体成员
  for (auto &member : structDecl->members) {
    if (member->getType() == ast::NodeType::VariableDecl) {
      analyzeVariableDecl(std::unique_ptr<ast::VariableDecl>(
          static_cast<ast::VariableDecl *>(member.release())));
    } else if (member->getType() == ast::NodeType::FunctionDecl) {
      analyzeFunctionDecl(std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(member.release())));
    }
  }

  // 退出结构体作用域
  symbolTable.exitScope();
}

// 分析枚举声明
void SemanticAnalyzer::analyzeEnumDecl(
    std::unique_ptr<ast::EnumDecl> enumDecl) {
  // 检查当前作用域是否已存在该枚举
  if (symbolTable.hasSymbolInCurrentScope(enumDecl->name)) {
    error(std::format("Enum '{}' already declared in current scope",
                      enumDecl->name),
          *enumDecl);
    return;
  }

  // 创建枚举类型
  // TODO: 实现枚举类型
  auto enumType =
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int);

  // 创建枚举符号
  Visibility visibility = parseVisibility(enumDecl->specifiers);
  auto enumSymbol =
      std::make_shared<EnumSymbol>(enumDecl->name, enumType, visibility);

  // 添加到符号表
  symbolTable.addSymbol(enumSymbol);

  // 分析枚举成员
  // 进入枚举作用域
  symbolTable.enterScope();

  // 分析枚举成员
  for (auto &member : enumDecl->members) {
    // 分析枚举成员值
    if (member->value) {
      analyzeExpression(std::unique_ptr<ast::Expression>(
          static_cast<ast::Expression *>(member->value.release())));
    }

    // 创建枚举成员符号
    auto memberSymbol =
        std::make_shared<VariableSymbol>(member->name, enumType, false);

    // 添加到符号表
    symbolTable.addSymbol(memberSymbol);
  }

  // 退出枚举作用域
  symbolTable.exitScope();
}

// 分析变量声明
void SemanticAnalyzer::analyzeVariableDecl(
    std::unique_ptr<ast::VariableDecl> varDecl) {
  // 检查当前作用域是否已存在该变量
  if (symbolTable.hasSymbolInCurrentScope(varDecl->name)) {
    error(std::format("Variable '{}' already declared in current scope",
                      varDecl->name),
          *varDecl);
    return;
  }

  // 分析变量类型
  std::shared_ptr<types::Type> varType;
  if (varDecl->type) {
    varType = analyzeType(static_cast<const ast::Type *>(varDecl->type.get()));
  } else if (varDecl->initializer) {
    varType = analyzeExpression(std::move(varDecl->initializer));
  } else {
    error(std::format("Variable '{}' has no type and no initializer",
                      varDecl->name),
          *varDecl);
    return;
  }

  // 创建变量符号
  bool isMutable = varDecl->isLate;
  bool isConst = varDecl->isConst;
  Visibility visibility = parseVisibility(varDecl->specifiers);
  auto varSymbol = std::make_shared<VariableSymbol>(
      varDecl->name, varType, isMutable, isConst, visibility);

  // 添加到符号表
  symbolTable.addSymbol(varSymbol);
}

// 分析函数声明
void SemanticAnalyzer::analyzeFunctionDecl(
    std::unique_ptr<ast::FunctionDecl> funcDecl) {
  // 检查当前作用域是否已存在该函数
  if (symbolTable.hasSymbolInCurrentScope(funcDecl->name)) {
    error(std::format("Function '{}' already declared in current scope",
                      funcDecl->name),
          *funcDecl);
    return;
  }

  // 分析参数类型
  std::vector<std::shared_ptr<types::Type>> paramTypes;
  for (const auto &param : funcDecl->params) {
    if (param->getType() == ast::NodeType::Parameter) {
      auto parameter = static_cast<const ast::Parameter *>(param.get());
      std::shared_ptr<types::Type> paramType;
      if (parameter->type) {
        paramType = analyzeType(parameter->type.get());
      } else {
        paramType = types::TypeFactory::getPrimitiveType(
            types::PrimitiveType::Kind::Int);
      }
      paramTypes.push_back(paramType);
    }
  }

  // 分析返回类型
  std::shared_ptr<types::Type> returnType;
  if (funcDecl->returnType) {
    returnType =
        analyzeType(static_cast<const ast::Type *>(funcDecl->returnType.get()));
  } else {
    returnType =
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
  }

  // 创建函数类型
  auto funcType = std::static_pointer_cast<types::FunctionType>(
      types::TypeFactory::getFunctionType(returnType, paramTypes));

  // 创建函数符号
  Visibility visibility = parseVisibility(funcDecl->specifiers);
  auto funcSymbol =
      std::make_shared<FunctionSymbol>(funcDecl->name, funcType, visibility);

  // 添加到符号表
  symbolTable.addSymbol(funcSymbol);

  // 分析函数体
  if (funcDecl->body) {
    // 进入函数作用域
    symbolTable.enterScope();

    // 分析函数参数
    for (const auto &param : funcDecl->params) {
      // 检查参数是否为Parameter类型
      if (param->getType() == ast::NodeType::Parameter) {
        auto parameter = static_cast<ast::Parameter *>(param.get());

        // 分析参数类型
        std::shared_ptr<types::Type> paramType;
        if (parameter->type) {
          paramType = analyzeType(
              static_cast<const ast::Type *>(parameter->type.get()));
        } else {
          // 类型推导（如果支持）
          paramType = types::TypeFactory::getPrimitiveType(
              types::PrimitiveType::Kind::Int);
        }

        // 创建参数符号
        auto paramSymbol =
            std::make_shared<VariableSymbol>(parameter->name, paramType, false);

        // 添加到符号表
        symbolTable.addSymbol(paramSymbol);
      }
    }

    // 分析函数体
    analyzeStatement(std::unique_ptr<ast::Statement>(
        static_cast<ast::Statement *>(funcDecl->body.release())));

    // 退出函数作用域
    symbolTable.exitScope();
  }
}

// 分析类声明
void SemanticAnalyzer::analyzeClassDecl(
    std::unique_ptr<ast::ClassDecl> classDecl) {
  // 检查当前作用域是否已存在该类
  if (symbolTable.hasSymbolInCurrentScope(classDecl->name)) {
    error(std::format("Class '{}' already declared in current scope",
                      classDecl->name),
          *classDecl);
    return;
  }

  // 创建类类型
  auto classType = std::static_pointer_cast<types::ClassType>(
      types::TypeFactory::getClassType(classDecl->name));

  // 创建类符号
  Visibility visibility = parseVisibility(classDecl->specifiers);
  auto classSymbol =
      std::make_shared<ClassSymbol>(classDecl->name, classType, visibility);

  // 添加到符号表
  symbolTable.addSymbol(classSymbol);

  // 分析类成员
  // 进入类作用域
  symbolTable.enterScope();

  // 分析类成员
  for (auto &member : classDecl->members) {
    switch (member->getType()) {
    case ast::NodeType::VariableDecl:
      analyzeVariableDecl(std::unique_ptr<ast::VariableDecl>(
          static_cast<ast::VariableDecl *>(member.release())));
      break;
    case ast::NodeType::FunctionDecl: {
      auto funcDecl = std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(member.release()));
      // 检查是否是构造函数（与类名相同）
      bool isConstructor = (funcDecl->name == classDecl->name);
      // 检查是否是析构函数（以 ~ 开头）
      bool isDestructor = (!funcDecl->name.empty() && funcDecl->name[0] == '~');
      // 分析函数声明
      analyzeFunctionDecl(std::move(funcDecl));
      break;
    }
    default:
      // 其他类型的成员，暂不处理
      break;
    }
  }

  // 退出类作用域
  symbolTable.exitScope();
}

// 分析语句
void SemanticAnalyzer::analyzeStatement(
    std::unique_ptr<ast::Statement> statement) {
  switch (statement->getType()) {
  case ast::NodeType::VariableDecl:
    analyzeVariableDecl(std::unique_ptr<ast::VariableDecl>(
        static_cast<ast::VariableStmt *>(statement.release())
            ->declaration.release()));
    break;
  case ast::NodeType::ExprStmt:
    analyzeExprStmt(std::unique_ptr<ast::ExprStmt>(
        static_cast<ast::ExprStmt *>(statement.release())));
    break;
  case ast::NodeType::CompoundStmt:
    analyzeCompoundStmt(std::unique_ptr<ast::CompoundStmt>(
        static_cast<ast::CompoundStmt *>(statement.release())));
    break;
  case ast::NodeType::ReturnStmt:
    analyzeReturnStmt(std::unique_ptr<ast::ReturnStmt>(
        static_cast<ast::ReturnStmt *>(statement.release())));
    break;
  case ast::NodeType::IfStmt:
    analyzeIfStmt(std::unique_ptr<ast::IfStmt>(
        static_cast<ast::IfStmt *>(statement.release())));
    break;
  case ast::NodeType::MatchStmt:
    analyzeMatchStmt(std::unique_ptr<ast::MatchStmt>(
        static_cast<ast::MatchStmt *>(statement.release())));
    break;
  case ast::NodeType::ForStmt:
    analyzeForStmt(std::unique_ptr<ast::ForStmt>(
        static_cast<ast::ForStmt *>(statement.release())));
    break;
  case ast::NodeType::WhileStmt:
    analyzeWhileStmt(std::unique_ptr<ast::WhileStmt>(
        static_cast<ast::WhileStmt *>(statement.release())));
    break;
  case ast::NodeType::DoWhileStmt:
    analyzeDoWhileStmt(std::unique_ptr<ast::DoWhileStmt>(
        static_cast<ast::DoWhileStmt *>(statement.release())));
    break;
  case ast::NodeType::BreakStmt:
    analyzeBreakStmt(std::unique_ptr<ast::BreakStmt>(
        static_cast<ast::BreakStmt *>(statement.release())));
    break;
  case ast::NodeType::ContinueStmt:
    analyzeContinueStmt(std::unique_ptr<ast::ContinueStmt>(
        static_cast<ast::ContinueStmt *>(statement.release())));
    break;
  case ast::NodeType::TryStmt:
    analyzeTryStmt(std::unique_ptr<ast::TryStmt>(
        static_cast<ast::TryStmt *>(statement.release())));
    break;
  case ast::NodeType::ThrowStmt:
    analyzeThrowStmt(std::unique_ptr<ast::ThrowStmt>(
        static_cast<ast::ThrowStmt *>(statement.release())));
    break;
  case ast::NodeType::DeferStmt:
    analyzeDeferStmt(std::unique_ptr<ast::DeferStmt>(
        static_cast<ast::DeferStmt *>(statement.release())));
    break;
  case ast::NodeType::ComptimeStmt:
    analyzeComptimeStmt(std::unique_ptr<ast::ComptimeStmt>(
        static_cast<ast::ComptimeStmt *>(statement.release())));
    break;
  default:
    // 其他类型的语句，暂不处理
    break;
  }
}

// 分析if语句
void SemanticAnalyzer::analyzeIfStmt(std::unique_ptr<ast::IfStmt> ifStmt) {
  // 分析条件表达式
  auto conditionType = analyzeExpression(std::move(ifStmt->condition));

  // 检查条件表达式是否为布尔类型
  if (!conditionType->isPrimitive() ||
      static_cast<const types::PrimitiveType *>(conditionType.get())
              ->getKind() != types::PrimitiveType::Kind::Bool) {
    error("Condition must be of boolean type", *ifStmt);
  }

  // 分析then分支
  analyzeStatement(std::move(ifStmt->thenBranch));

  // 分析else分支
  if (ifStmt->elseBranch) {
    analyzeStatement(std::move(ifStmt->elseBranch));
  }
}

// 分析match语句
void SemanticAnalyzer::analyzeMatchStmt(
    std::unique_ptr<ast::MatchStmt> matchStmt) {
  // 分析匹配表达式
  analyzeExpression(std::move(matchStmt->expr));

  // 分析每个匹配臂
  for (auto &arm : matchStmt->arms) {
    // 分析模式（暂时跳过，需要实现模式分析）
    // 分析守卫表达式
    if (arm->guard) {
      auto guardType = analyzeExpression(std::move(arm->guard));

      // 检查守卫表达式是否为布尔类型
      if (!guardType->isPrimitive() ||
          static_cast<const types::PrimitiveType *>(guardType.get())
                  ->getKind() != types::PrimitiveType::Kind::Bool) {
        error("Guard expression must be of boolean type", *arm);
      }
    }

    // 分析主体语句
    analyzeStatement(std::move(arm->body));
  }
}

// 分析for语句
void SemanticAnalyzer::analyzeForStmt(std::unique_ptr<ast::ForStmt> forStmt) {
  // 进入循环作用域
  symbolTable.enterScope();
  scopeStack.push_back("loop");

  // 分析初始化语句
  if (forStmt->init) {
    // 根据初始化语句类型进行分析
    if (forStmt->init->getType() == ast::NodeType::VariableDecl) {
      analyzeVariableDecl(std::unique_ptr<ast::VariableDecl>(
          static_cast<ast::VariableDecl *>(forStmt->init.release())));
    } else if (forStmt->init->getType() == ast::NodeType::ExprStmt) {
      analyzeExprStmt(std::unique_ptr<ast::ExprStmt>(
          static_cast<ast::ExprStmt *>(forStmt->init.release())));
    }
  }

  // 分析条件表达式
  if (forStmt->condition) {
    auto conditionType = analyzeExpression(std::move(forStmt->condition));

    // 检查条件表达式是否为布尔类型
    if (!conditionType->isPrimitive() ||
        static_cast<const types::PrimitiveType *>(conditionType.get())
                ->getKind() != types::PrimitiveType::Kind::Bool) {
      error("Condition must be of boolean type", *forStmt);
    }
  }

  // 分析更新表达式
  if (forStmt->update) {
    analyzeExpression(std::move(forStmt->update));
  }

  // 分析循环体
  analyzeStatement(std::move(forStmt->body));

  // 退出循环作用域
  scopeStack.pop_back();
  symbolTable.exitScope();
}

// 分析while语句
void SemanticAnalyzer::analyzeWhileStmt(
    std::unique_ptr<ast::WhileStmt> whileStmt) {
  // 进入循环作用域
  symbolTable.enterScope();
  scopeStack.push_back("loop");

  // 分析条件表达式
  auto conditionType = analyzeExpression(std::move(whileStmt->condition));

  // 检查条件表达式是否为布尔类型
  if (!conditionType->isPrimitive() ||
      static_cast<const types::PrimitiveType *>(conditionType.get())
              ->getKind() != types::PrimitiveType::Kind::Bool) {
    error("Condition must be of boolean type", *whileStmt);
  }

  // 分析循环体
  analyzeStatement(std::move(whileStmt->body));

  // 退出循环作用域
  scopeStack.pop_back();
  symbolTable.exitScope();
}

// 分析do-while语句
void SemanticAnalyzer::analyzeDoWhileStmt(
    std::unique_ptr<ast::DoWhileStmt> doWhileStmt) {
  // 进入循环作用域
  symbolTable.enterScope();
  scopeStack.push_back("loop");

  // 分析循环体
  analyzeStatement(std::move(doWhileStmt->body));

  // 分析条件表达式
  auto conditionType = analyzeExpression(std::move(doWhileStmt->condition));

  // 检查条件表达式是否为布尔类型
  if (!conditionType->isPrimitive() ||
      static_cast<const types::PrimitiveType *>(conditionType.get())
              ->getKind() != types::PrimitiveType::Kind::Bool) {
    error("Condition must be of boolean type", *doWhileStmt);
  }

  // 退出循环作用域
  scopeStack.pop_back();
  symbolTable.exitScope();
}

// 分析break语句
void SemanticAnalyzer::analyzeBreakStmt(
    std::unique_ptr<ast::BreakStmt> breakStmt) {
  // 检查是否在循环或switch语句中
  bool inValidScope = false;
  for (const auto &scope : scopeStack) {
    if (scope == "loop" || scope == "switch") {
      inValidScope = true;
      break;
    }
  }

  if (!inValidScope) {
    error("break statement not inside a loop or switch", *breakStmt);
  }
}

// 分析continue语句
void SemanticAnalyzer::analyzeContinueStmt(
    std::unique_ptr<ast::ContinueStmt> continueStmt) {
  // 检查是否在循环语句中
  bool inLoopScope = false;
  for (const auto &scope : scopeStack) {
    if (scope == "loop") {
      inLoopScope = true;
      break;
    }
  }

  if (!inLoopScope) {
    error("continue statement not inside a loop", *continueStmt);
  }
}

// 分析try语句
void SemanticAnalyzer::analyzeTryStmt(std::unique_ptr<ast::TryStmt> tryStmt) {
  // 进入try作用域
  symbolTable.enterScope();

  // 分析try块
  if (tryStmt->tryBlock) {
    analyzeStatement(std::move(tryStmt->tryBlock));
  }

  // 退出try作用域
  symbolTable.exitScope();

  // 分析catch块
  for (auto &catchStmt : tryStmt->catchStmts) {
    // 进入catch作用域
    symbolTable.enterScope();

    // 分析catch参数
    if (catchStmt->param) {
      // 检查是否是 catch(...)
      if (catchStmt->param->name != "...") {
        // 正常的 catch(Type var)
        auto parameter = catchStmt->param.get();
        if (parameter->type) {
          auto paramType = analyzeType(
              static_cast<const ast::Type *>(parameter->type.get()));

          // 创建参数符号
          auto paramSymbol = std::make_shared<VariableSymbol>(parameter->name,
                                                              paramType, false);
          symbolTable.addSymbol(paramSymbol);
        }
      }
    }

    // 分析catch块
    if (catchStmt->body) {
      analyzeStatement(std::move(catchStmt->body));
    }

    // 退出catch作用域
    symbolTable.exitScope();
  }
}

// 分析throw语句
void SemanticAnalyzer::analyzeThrowStmt(
    std::unique_ptr<ast::ThrowStmt> throwStmt) {
  // 分析throw表达式（如果有）
  if (throwStmt->expr) {
    analyzeExpression(std::move(throwStmt->expr));
  }
}

// 分析defer语句
void SemanticAnalyzer::analyzeDeferStmt(
    std::unique_ptr<ast::DeferStmt> deferStmt) {
  // 分析defer表达式
  analyzeExpression(std::move(deferStmt->expr));
}

// 分析comptime语句
void SemanticAnalyzer::analyzeComptimeStmt(
    std::unique_ptr<ast::ComptimeStmt> comptimeStmt) {
  // 分析comptime语句中的内容
  analyzeStatement(std::move(comptimeStmt->stmt));
}

// 分析表达式语句
void SemanticAnalyzer::analyzeExprStmt(
    std::unique_ptr<ast::ExprStmt> exprStmt) {
  // 分析表达式
  analyzeExpression(std::move(exprStmt->expr));
}

// 分析复合语句
void SemanticAnalyzer::analyzeCompoundStmt(
    std::unique_ptr<ast::CompoundStmt> compoundStmt) {
  // 进入复合语句作用域
  symbolTable.enterScope();

  // 分析每个语句
  for (auto &statement : compoundStmt->statements) {
    analyzeStatement(std::unique_ptr<ast::Statement>(
        static_cast<ast::Statement *>(statement.release())));
  }

  // 退出复合语句作用域
  symbolTable.exitScope();
}

// 分析返回语句
void SemanticAnalyzer::analyzeReturnStmt(
    std::unique_ptr<ast::ReturnStmt> returnStmt) {
  if (returnStmt->expr) {
    // 分析返回表达式
    analyzeExpression(std::move(returnStmt->expr));
    // TODO: 检查返回类型是否与函数返回类型兼容
  }
}

// 分析表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeExpression(
    std::unique_ptr<ast::Expression> expression) {
  switch (expression->getType()) {
  case ast::NodeType::BinaryExpr:
    return analyzeBinaryExpr(std::unique_ptr<ast::BinaryExpr>(
        static_cast<ast::BinaryExpr *>(expression.release())));
  case ast::NodeType::UnaryExpr:
    return analyzeUnaryExpr(std::unique_ptr<ast::UnaryExpr>(
        static_cast<ast::UnaryExpr *>(expression.release())));
  case ast::NodeType::Identifier:
    return analyzeIdentifierExpr(std::unique_ptr<ast::Identifier>(
        static_cast<ast::Identifier *>(expression.release())));
  case ast::NodeType::Literal:
    return analyzeLiteralExpr(std::unique_ptr<ast::Literal>(
        static_cast<ast::Literal *>(expression.release())));
  case ast::NodeType::CallExpr:
    return analyzeCallExpr(std::unique_ptr<ast::CallExpr>(
        static_cast<ast::CallExpr *>(expression.release())));
  case ast::NodeType::MemberExpr:
    return analyzeMemberExpr(std::unique_ptr<ast::MemberExpr>(
        static_cast<ast::MemberExpr *>(expression.release())));
  case ast::NodeType::SubscriptExpr:
    return analyzeSubscriptExpr(std::unique_ptr<ast::SubscriptExpr>(
        static_cast<ast::SubscriptExpr *>(expression.release())));
  case ast::NodeType::NewExpr:
    return analyzeNewExpr(std::unique_ptr<ast::NewExpr>(
        static_cast<ast::NewExpr *>(expression.release())));
  case ast::NodeType::DeleteExpr:
    return analyzeDeleteExpr(std::unique_ptr<ast::DeleteExpr>(
        static_cast<ast::DeleteExpr *>(expression.release())));
  case ast::NodeType::ThisExpr:
    return analyzeThisExpr(std::unique_ptr<ast::ThisExpr>(
        static_cast<ast::ThisExpr *>(expression.release())));
  case ast::NodeType::SelfExpr:
    return analyzeSelfExpr(std::unique_ptr<ast::SelfExpr>(
        static_cast<ast::SelfExpr *>(expression.release())));
  case ast::NodeType::SuperExpr:
    return analyzeSuperExpr(std::unique_ptr<ast::SuperExpr>(
        static_cast<ast::SuperExpr *>(expression.release())));
  case ast::NodeType::ExpansionExpr:
    return analyzeExpansionExpr(std::unique_ptr<ast::ExpansionExpr>(
        static_cast<ast::ExpansionExpr *>(expression.release())));
  case ast::NodeType::LambdaExpr:
    return analyzeLambdaExpr(std::unique_ptr<ast::LambdaExpr>(
        static_cast<ast::LambdaExpr *>(expression.release())));
  case ast::NodeType::ArrayInitExpr:
    return analyzeArrayInitExpr(std::unique_ptr<ast::ArrayInitExpr>(
        static_cast<ast::ArrayInitExpr *>(expression.release())));
  case ast::NodeType::StructInitExpr:
    return analyzeStructInitExpr(std::unique_ptr<ast::StructInitExpr>(
        static_cast<ast::StructInitExpr *>(expression.release())));
  case ast::NodeType::TupleExpr:
    return analyzeTupleExpr(std::unique_ptr<ast::TupleExpr>(
        static_cast<ast::TupleExpr *>(expression.release())));
  default:
    // 其他类型的表达式，暂不处理
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }
}

// 分析new表达式
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeNewExpr(std::unique_ptr<ast::NewExpr> newExpr) {
  // 分析类型
  auto type = analyzeType(static_cast<const ast::Type *>(newExpr->type.get()));

  // 分析参数
  for (auto &arg : newExpr->args) {
    analyzeExpression(std::move(arg));
  }

  // 返回指针类型
  return types::TypeFactory::getPointerType(type);
}

// 分析delete表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeDeleteExpr(
    std::unique_ptr<ast::DeleteExpr> deleteExpr) {
  // 分析表达式
  auto exprType = analyzeExpression(std::move(deleteExpr->expr));

  // 检查表达式是否为指针类型
  if (!exprType->isPointer()) {
    error("Operand of delete must be a pointer", *deleteExpr);
  }

  // 返回void类型
  return types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
}

// 分析this表达式
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeThisExpr(std::unique_ptr<ast::ThisExpr> thisExpr) {
  // TODO: 返回当前类的指针类型
  return types::TypeFactory::getPointerType(
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int));
}

// 分析self表达式
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr) {
  // TODO: 返回当前类的引用类型
  return types::TypeFactory::getPointerType(
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int));
}

// 分析super表达式
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSuperExpr(std::unique_ptr<ast::SuperExpr> superExpr) {
  // TODO: 返回父类的指针类型
  return types::TypeFactory::getPointerType(
      types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int));
}

// 分析二元表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeBinaryExpr(
    std::unique_ptr<ast::BinaryExpr> binaryExpr) {
  // 分析左操作数
  auto leftType = analyzeExpression(std::move(binaryExpr->left));

  // 分析右操作数
  auto rightType = analyzeExpression(std::move(binaryExpr->right));

  // 检查是否是赋值操作符（包括 =, +=, -= 等）
  bool isAssignment = (binaryExpr->op == ast::BinaryExpr::Op::Assign ||
                       binaryExpr->op == ast::BinaryExpr::Op::AddAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::SubAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::MulAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::DivAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::ModAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::AndAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::OrAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::XorAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::ShlAssign ||
                       binaryExpr->op == ast::BinaryExpr::Op::ShrAssign);

  // 如果是赋值操作，检查左操作数是否是只读类型
  if (isAssignment && leftType->isReadonly()) {
    error("Cannot assign to readonly type", *binaryExpr);
  }

  // 检查类型兼容性
  if (!rightType->isCompatibleWith(*leftType)) {
    error("Type mismatch in binary expression", *binaryExpr);
  }

  // 根据操作符返回相应类型
  // 比较运算符返回布尔类型
  switch (binaryExpr->op) {
  case ast::BinaryExpr::Op::Eq:
  case ast::BinaryExpr::Op::Ne:
  case ast::BinaryExpr::Op::Lt:
  case ast::BinaryExpr::Op::Le:
  case ast::BinaryExpr::Op::Gt:
  case ast::BinaryExpr::Op::Ge:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Bool);
  case ast::BinaryExpr::Op::LogicAnd:
  case ast::BinaryExpr::Op::LogicOr:
    // 逻辑运算符要求操作数为布尔类型
    if (!leftType->isPrimitive() ||
        static_cast<const types::PrimitiveType *>(leftType.get())->getKind() !=
            types::PrimitiveType::Kind::Bool) {
      error("Logical operators require boolean operands", *binaryExpr);
    }
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Bool);
  default:
    // 其他运算符返回操作数类型
    return leftType;
  }
}

// 分析一元表达式
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr) {
  // 分析操作数
  auto operandType = analyzeExpression(std::move(unaryExpr->expr));

  // 根据操作符返回相应类型
  switch (unaryExpr->op) {
  case ast::UnaryExpr::Op::Not:
    // 逻辑非要求操作数为布尔类型
    if (!operandType->isPrimitive() ||
        static_cast<const types::PrimitiveType *>(operandType.get())
                ->getKind() != types::PrimitiveType::Kind::Bool) {
      error("Logical NOT requires boolean operand", *unaryExpr);
    }
    return operandType;
  case ast::UnaryExpr::Op::AddressOf:
    // 取地址返回指针类型
    return types::TypeFactory::getPointerType(operandType);
  case ast::UnaryExpr::Op::Dereference:
    // 解引用要求操作数为指针类型
    if (!operandType->isPointer()) {
      error("Dereference requires pointer operand", *unaryExpr);
    }
    return std::static_pointer_cast<types::PointerType>(operandType)
        ->getPointeeType();
  case ast::UnaryExpr::Op::Plus:
  case ast::UnaryExpr::Op::Minus:
    // 正负号要求操作数为数值类型
    if (!operandType->isPrimitive()) {
      error("Unary operator requires primitive type operand", *unaryExpr);
    }
    return operandType;
  case ast::UnaryExpr::Op::Move:
    // 移动操作返回相同类型
    return operandType;
  case ast::UnaryExpr::Op::Immutable:
    // 不可变修饰符返回相同类型
    return operandType;
  case ast::UnaryExpr::Op::At:
    // @反射操作符
    return operandType;
  default:
    return operandType;
  }
}

// 分析标识符表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeIdentifierExpr(
    std::unique_ptr<ast::Identifier> identifier) {
  // 查找标识符
  auto symbol = symbolTable.lookupSymbol(identifier->name);
  if (!symbol) {
    error(std::format("Undefined identifier '{}'", identifier->name),
          *identifier);
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }

  // 根据符号类型返回相应类型
  switch (symbol->getType()) {
  case SymbolType::Variable:
    return std::static_pointer_cast<VariableSymbol>(symbol)->getType();
  case SymbolType::Function: {
    auto funcType = std::static_pointer_cast<FunctionSymbol>(symbol)->getType();
    return types::TypeFactory::getPointerType(funcType);
  }
  case SymbolType::Class:
    return std::static_pointer_cast<ClassSymbol>(symbol)->getType();
  default:
    // 其他类型的符号，暂不处理
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }
}

// 分析字面量表达式
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeLiteralExpr(std::unique_ptr<ast::Literal> literal) {
  // 根据字面量类型返回相应类型
  switch (literal->type) {
  case ast::Literal::Type::Integer:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Int);
  case ast::Literal::Type::Floating:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Double);
  case ast::Literal::Type::Boolean:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Bool);
  case ast::Literal::Type::Character:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Char);
  case ast::Literal::Type::String:
    return types::TypeFactory::getLiteralViewType();
  case ast::Literal::Type::Null:
    // TODO: 返回可空类型
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  default:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }
}

// 分析函数调用表达式
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeCallExpr(std::unique_ptr<ast::CallExpr> callExpr) {
  // 分析函数表达式
  auto calleeType = analyzeExpression(std::move(callExpr->callee));

  // 分析实参
  std::vector<std::shared_ptr<types::Type>> argTypes;
  for (auto &arg : callExpr->args) {
    argTypes.push_back(analyzeExpression(std::move(arg)));
  }

  std::shared_ptr<types::FunctionType> functionType;

  // 检查函数类型是否正确
  if (calleeType->isFunction()) {
    functionType = std::static_pointer_cast<types::FunctionType>(calleeType);
  } else if (calleeType->isPointer()) {
    auto pointerType = std::static_pointer_cast<types::PointerType>(calleeType);
    if (pointerType->getPointeeType()->isFunction()) {
      functionType = std::static_pointer_cast<types::FunctionType>(
          pointerType->getPointeeType());
    }
  }

  if (!functionType) {
    error("Callee is not a function or function pointer", *callExpr);
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }

  // 检查实参类型是否与形参类型匹配
  const auto &paramTypes = functionType->getParameterTypes();
  if (argTypes.size() != paramTypes.size()) {
    error(std::format("Expected {} arguments, got {}", paramTypes.size(),
                      argTypes.size()),
          *callExpr);
  }

  for (size_t i = 0; i < std::min(argTypes.size(), paramTypes.size()); ++i) {
    if (!argTypes[i]->isCompatibleWith(*paramTypes[i])) {
      error(std::format("Argument {} type mismatch", i + 1), *callExpr);
    }
  }

  return functionType->getReturnType();
}

// 分析成员访问表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeMemberExpr(
    std::unique_ptr<ast::MemberExpr> memberExpr) {
  // 分析对象表达式
  auto objType = analyzeExpression(std::move(memberExpr->object));

  // 检查对象是否是 LiteralView
  if (objType->isLiteralView()) {
    if (memberExpr->member == "ptr") {
      // 返回 byte!^（指向只读 byte 的指针）
      auto byteType = types::TypeFactory::getPrimitiveType(
          types::PrimitiveType::Kind::Byte);
      auto readonlyByteType = types::TypeFactory::getReadonlyType(byteType);
      return types::TypeFactory::getPointerType(readonlyByteType);
    } else if (memberExpr->member == "len") {
      // 返回 long
      return types::TypeFactory::getPrimitiveType(
          types::PrimitiveType::Kind::Long);
    } else {
      error(std::format("LiteralView has no member '{}'", memberExpr->member),
            *memberExpr);
      return types::TypeFactory::getPrimitiveType(
          types::PrimitiveType::Kind::Void);
    }
  }

  // 检查对象类型是否为类或结构体
  if (!objType->isClass()) {
    error("Member access on non-class type", *memberExpr);
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }

  // TODO: 查找成员并返回其类型
  // 暂时返回 int
  return types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int);
}

// 分析下标访问表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeSubscriptExpr(
    std::unique_ptr<ast::SubscriptExpr> subscriptExpr) {
  // 分析对象表达式
  auto objectType = analyzeExpression(std::move(subscriptExpr->object));

  // 分析索引表达式
  auto indexType = analyzeExpression(std::move(subscriptExpr->index));

  // 检查索引表达式是否为整数类型
  if (!indexType->isPrimitive() ||
      static_cast<const types::PrimitiveType *>(indexType.get())->getKind() !=
          types::PrimitiveType::Kind::Int) {
    error("Array index must be an integer", *subscriptExpr);
  }

  // 如果是数组类型，返回元素类型
  if (objectType->isArray()) {
    return std::static_pointer_cast<types::ArrayType>(objectType)
        ->getElementType();
  }

  // 如果是切片类型，返回元素类型
  if (objectType->isSlice()) {
    return std::static_pointer_cast<types::SliceType>(objectType)
        ->getElementType();
  }

  // 否则报错
  error("Subscript on non-array type", *subscriptExpr);
  return types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
}

// 分析可变参数展开表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeExpansionExpr(
    std::unique_ptr<ast::ExpansionExpr> expansionExpr) {
  // 分析被展开的表达式
  auto exprType = analyzeExpression(std::move(expansionExpr->expr));

  // 可变参数展开的类型与被展开的表达式类型相同
  // 但在实际使用中，它会被展开为多个参数
  return exprType;
}

// 分析类型
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeType(const ast::Type *type) {
  if (!type) {
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }

  switch (type->getType()) {
  case ast::NodeType::PrimitiveType: {
    auto primitiveType = static_cast<const ast::PrimitiveType *>(type);
    // 转换ast::PrimitiveType::Kind到types::PrimitiveType::Kind
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
      kind = types::PrimitiveType::Kind::Void;
      break;
    }
    return types::TypeFactory::getPrimitiveType(kind);
  }
  case ast::NodeType::PointerType: {
    auto pointerType = static_cast<const ast::PointerType *>(type);
    // 检查PointerType是否有baseType成员
    if (pointerType->baseType) {
      auto pointeeType = analyzeType(pointerType->baseType.get());
      return types::TypeFactory::getPointerType(pointeeType);
    }
    return types::TypeFactory::getPointerType(
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void));
  }
  case ast::NodeType::ArrayType: {
    auto arrayType = static_cast<const ast::ArrayType *>(type);
    // 检查ArrayType是否有baseType成员
    if (arrayType->baseType) {
      auto elementType = analyzeType(arrayType->baseType.get());
      return types::TypeFactory::getArrayType(elementType,
                                              0); // 暂时使用0作为大小
    }
    return types::TypeFactory::getArrayType(
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void),
        0);
  }
  case ast::NodeType::SliceType: {
    auto sliceType = static_cast<const ast::SliceType *>(type);
    // 检查SliceType是否有baseType成员
    if (sliceType->baseType) {
      auto elementType = analyzeType(sliceType->baseType.get());
      return types::TypeFactory::getSliceType(elementType);
    }
    return types::TypeFactory::getSliceType(
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void));
  }
  case ast::NodeType::RectangularArrayType: {
    auto rectArrayType = static_cast<const ast::RectangularArrayType *>(type);
    if (rectArrayType->baseType) {
      auto elementType = analyzeType(rectArrayType->baseType.get());
      std::vector<size_t> sizes;
      for (const auto &sizeExpr : rectArrayType->sizes) {
        sizes.push_back(0);
      }
      return types::TypeFactory::getRectangularArrayType(elementType, sizes);
    }
    return types::TypeFactory::getRectangularArrayType(
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void),
        {});
  }
  case ast::NodeType::RectangularSliceType: {
    auto rectSliceType = static_cast<const ast::RectangularSliceType *>(type);
    if (rectSliceType->baseType) {
      auto elementType = analyzeType(rectSliceType->baseType.get());
      return types::TypeFactory::getRectangularSliceType(elementType,
                                                         rectSliceType->rank);
    }
    return types::TypeFactory::getRectangularSliceType(
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void),
        1);
  }
  case ast::NodeType::NamedType: {
    auto namedType = static_cast<const ast::NamedType *>(type);
    // 查找符号表中的类/结构体或类型别名
    auto symbol = symbolTable.lookupSymbol(namedType->name);
    if (symbol) {
      if (symbol->getType() == SymbolType::Class) {
        return std::static_pointer_cast<ClassSymbol>(symbol)->getType();
      } else if (symbol->getType() == SymbolType::TypeAlias) {
        return std::static_pointer_cast<TypeAliasSymbol>(symbol)->getType();
      }
    }
    error(std::format("Undefined type '{}'", namedType->name), *namedType);
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }
  case ast::NodeType::FunctionType: {
    auto funcType = static_cast<const ast::FunctionType *>(type);
    // 分析参数类型
    std::vector<std::shared_ptr<types::Type>> paramTypes;
    for (const auto &paramType : funcType->parameterTypes) {
      paramTypes.push_back(analyzeType(paramType.get()));
    }
    // 分析返回类型
    auto returnType = analyzeType(funcType->returnType.get());
    // 创建函数类型
    return types::TypeFactory::getFunctionType(returnType, paramTypes);
  }
  case ast::NodeType::ReadonlyType: {
    auto readonlyType = static_cast<const ast::ReadonlyType *>(type);
    auto baseType = analyzeType(readonlyType->baseType.get());
    return types::TypeFactory::getReadonlyType(baseType);
  }
  default:
    return types::TypeFactory::getPrimitiveType(
        types::PrimitiveType::Kind::Void);
  }
}

// 检查类型兼容性
bool SemanticAnalyzer::checkTypeCompatibility(const types::Type &type1,
                                              const types::Type &type2) {
  return type1.isCompatibleWith(type2);
}

// 报告错误
void SemanticAnalyzer::error(const std::string &message,
                             const ast::Node &node) {
  std::cerr << "Semantic error: " << message << std::endl;
  // TODO: 添加更多错误信息，如行号、列号等
}

// 分析类型别名声明
void SemanticAnalyzer::analyzeTypeAliasDecl(
    std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl) {
  // 检查当前作用域是否已存在该类型别名
  if (symbolTable.hasSymbolInCurrentScope(typeAliasDecl->name)) {
    error(std::format("Type alias '{}' already declared in current scope",
                      typeAliasDecl->name),
          *typeAliasDecl);
    return;
  }

  // 分析被别名的类型
  auto aliasedType =
      analyzeType(static_cast<const ast::Type *>(typeAliasDecl->type.get()));

  // 创建类型别名符号
  Visibility visibility = parseVisibility(typeAliasDecl->specifiers);
  auto typeAliasSymbol = std::make_shared<TypeAliasSymbol>(
      typeAliasDecl->name, aliasedType, visibility);

  // 添加到符号表
  symbolTable.addSymbol(typeAliasSymbol);
}

// 分析模块声明
void SemanticAnalyzer::analyzeModuleDecl(
    std::unique_ptr<ast::ModuleDecl> moduleDecl) {
  std::string moduleName;
  for (size_t i = 0; i < moduleDecl->modulePath.size(); ++i) {
    if (i > 0)
      moduleName += ".";
    moduleName += moduleDecl->modulePath[i];
  }

  std::string symbolName = "module:" + moduleName;
  if (symbolTable.hasSymbolInCurrentScope(symbolName)) {
    error(std::format("Module '{}' already declared", moduleName), *moduleDecl);
    return;
  }

  auto moduleSymbol = std::make_shared<ModuleSymbol>(symbolName);
  moduleSymbol->modulePath = moduleDecl->modulePath;
  symbolTable.addSymbol(moduleSymbol);
}

// 分析导入声明
void SemanticAnalyzer::analyzeImportDecl(
    std::unique_ptr<ast::ImportDecl> importDecl) {
  std::string modulePath;
  for (size_t i = 0; i < importDecl->modulePath.size(); ++i) {
    if (i > 0)
      modulePath += ".";
    modulePath += importDecl->modulePath[i];
  }

  Visibility visibility = parseVisibility(importDecl->specifiers);

  // TODO: 实际解析模块并导入符号
  // 目前只是创建一个导入记录
  auto moduleSymbol = std::make_shared<ModuleSymbol>(modulePath, visibility);
  moduleSymbol->modulePath = importDecl->modulePath;
  // 不添加到符号表，因为这是导入的模块，不是当前模块定义的
}

// 分析扩展声明
void SemanticAnalyzer::analyzeExtensionDecl(
    std::unique_ptr<ast::ExtensionDecl> extensionDecl) {
  // 首先分析被扩展的类型
  auto extendedType = analyzeType(extensionDecl->extendedType.get());
  if (!extendedType) {
    error("Failed to analyze extended type", *extensionDecl);
    return;
  }

  // 进入新的作用域来分析扩展的成员
  symbolTable.enterScope();

  // 分析所有成员声明
  for (auto &member : extensionDecl->members) {
    // 检查是否是声明类型
    auto nodeType = member->getType();
    if (nodeType == ast::NodeType::VariableDecl ||
        nodeType == ast::NodeType::FunctionDecl ||
        nodeType == ast::NodeType::ClassDecl ||
        nodeType == ast::NodeType::StructDecl ||
        nodeType == ast::NodeType::EnumDecl ||
        nodeType == ast::NodeType::TypeAliasDecl ||
        nodeType == ast::NodeType::GetterDecl ||
        nodeType == ast::NodeType::SetterDecl) {
      analyzeDeclaration(std::unique_ptr<ast::Declaration>(
          static_cast<ast::Declaration *>(member.release())));
    }
  }

  // 离开作用域
  symbolTable.exitScope();
}

// 分析 Getter 声明
void SemanticAnalyzer::analyzeGetterDecl(
    std::unique_ptr<ast::GetterDecl> getterDecl) {
  // TODO: 更详细的语义分析，比如分析返回类型、函数体等
  // 暂时只做基本处理
}

// 分析 Setter 声明
void SemanticAnalyzer::analyzeSetterDecl(
    std::unique_ptr<ast::SetterDecl> setterDecl) {
  // TODO: 更详细的语义分析，比如分析参数类型、函数体等
  // 暂时只做基本处理
}

// 分析Lambda表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeLambdaExpr(
    std::unique_ptr<ast::LambdaExpr> lambdaExpr) {
  // 分析参数类型
  std::vector<std::shared_ptr<types::Type>> paramTypes;
  for (const auto &param : lambdaExpr->params) {
    std::shared_ptr<types::Type> paramType;
    if (param->type) {
      paramType =
          analyzeType(static_cast<const ast::Type *>(param->type.get()));
    } else {
      // 类型推导
      paramType =
          types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Int);
    }
    paramTypes.push_back(paramType);
  }

  // 分析Lambda体以推导返回类型
  std::shared_ptr<types::Type> returnType;
  if (lambdaExpr->body) {
    if (lambdaExpr->body->getType() == ast::NodeType::ExprStmt) {
      auto exprStmt = static_cast<ast::ExprStmt *>(lambdaExpr->body.get());
      returnType = analyzeExpression(std::unique_ptr<ast::Expression>(
          static_cast<ast::Expression *>(exprStmt->expr.release())));
    } else {
      // 复合语句，返回类型为void
      returnType = types::TypeFactory::getPrimitiveType(
          types::PrimitiveType::Kind::Void);
    }
  } else {
    returnType =
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
  }

  // 创建函数类型
  return types::TypeFactory::getFunctionType(returnType, paramTypes);
}

// 分析数组初始化表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeArrayInitExpr(
    std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr) {
  // 分析每个元素表达式
  std::vector<std::shared_ptr<types::Type>> elementTypes;
  for (auto &element : arrayInitExpr->elements) {
    auto elementType = analyzeExpression(std::move(element));
    elementTypes.push_back(elementType);
  }

  // 检查所有元素类型是否一致
  if (!elementTypes.empty()) {
    auto firstType = elementTypes[0];
    for (size_t i = 1; i < elementTypes.size(); ++i) {
      if (!firstType->isCompatibleWith(*elementTypes[i])) {
        error("Array elements must have the same type", *arrayInitExpr);
        break;
      }
    }
  }

  // 返回数组类型
  if (elementTypes.empty()) {
    return types::TypeFactory::getArrayType(
        types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void),
        0);
  }

  return types::TypeFactory::getArrayType(
      elementTypes[0], static_cast<int>(elementTypes.size()));
}

// 分析结构体初始化表达式
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeStructInitExpr(
    std::unique_ptr<ast::StructInitExpr> structInitExpr) {
  // 分析每个字段表达式
  for (auto &field : structInitExpr->fields) {
    analyzeExpression(std::move(field.second));
  }

  // TODO: 根据结构体类型返回正确的类型
  // 暂时返回void类型
  return types::TypeFactory::getPrimitiveType(types::PrimitiveType::Kind::Void);
}

// 分析元组表达式
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTupleExpr(std::unique_ptr<ast::TupleExpr> tupleExpr) {
  std::vector<std::shared_ptr<types::Type>> elementTypes;

  // 分析每个元素表达式
  for (auto &element : tupleExpr->elements) {
    auto elementType = analyzeExpression(std::move(element));
    elementTypes.push_back(elementType);
  }

  // 返回元组类型
  return types::TypeFactory::getTupleType(std::move(elementTypes));
}

// 分析外部声明块
void SemanticAnalyzer::analyzeExternDecl(
    std::unique_ptr<ast::ExternDecl> externDecl) {
  // 分析extern块中的所有声明
  for (auto &declaration : externDecl->declarations) {
    analyzeDeclaration(std::move(declaration));
  }
}

} // namespace semantic
} // namespace c_hat
