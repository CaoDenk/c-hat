#include "Parser.h"
#include <format>
#include <iostream>
#include <stdexcept>

namespace c_hat {
namespace parser {

// Parser构造函数
Parser::Parser(std::string source) : lexer(std::move(source)) { advance(); }

// 解析整个程序
std::unique_ptr<ast::Program> Parser::parseProgram() {
  std::vector<std::unique_ptr<ast::Declaration>> declarations;

  while (!check(lexer::TokenType::EndOfFile)) {
    if (auto decl = parseDeclaration()) {
      declarations.push_back(std::move(decl));
    } else {
      advance();
    }
  }

  return std::make_unique<ast::Program>(std::move(declarations));
}

std::unique_ptr<ast::Expression> Parser::parseExpressionOnly() {
  advance();
  auto expr = parseExpression();
  return expr;
}

std::unique_ptr<ast::Statement> Parser::parseStatementOnly() {
  advance();
  auto stmt = parseStatement();
  return stmt;
}

std::unique_ptr<ast::Declaration> Parser::parseDeclarationOnly() {
  advance();
  auto decl = parseDeclaration();
  return decl;
}

// 错误处理
void Parser::error(const std::string &message) {
  std::string errorMsg = message;

  if (currentToken) {
    errorMsg += std::format(" at line {}, column {}", currentToken->getLine(),
                            currentToken->getColumn());
  } else if (previousToken) {
    errorMsg += std::format(" after '{}'", previousToken->getValue());
  }

  throw std::runtime_error(errorMsg);
}

// 消费下一个词法单元
void Parser::advance() {
  previousToken = currentToken;
  currentToken = lexer.nextToken();
}

// 检查当前词法单元类型
bool Parser::check(lexer::TokenType type) const {
  if (!currentToken)
    return false;
  return currentToken->getType() == type;
}

// 匹配并消费指定类型的词法单元
bool Parser::match(lexer::TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }
  return false;
}

// 期望并消费指定类型的词法单元
void Parser::expect(lexer::TokenType type, const std::string &message) {
  if (!match(type)) {
    error(message);
  }
}

// 解析声明
std::unique_ptr<ast::Declaration> Parser::parseDeclaration() {
  // 解析属性列表
  std::vector<std::unique_ptr<ast::AttributeApplication>> attributes;
  while (check(lexer::TokenType::LBracket)) {
    auto attr = parseAttributeApplication();
    if (attr) {
      attributes.push_back(std::move(attr));
    }
  }

  // 辅助函数：将属性附加到声明
  auto attachAttributes = [&attributes](std::unique_ptr<ast::Declaration> decl)
      -> std::unique_ptr<ast::Declaration> {
    if (decl && !attributes.empty()) {
      decl->attributes = std::move(attributes);
    }
    return decl;
  };

  // 模块声明必须在最前面
  if (match(lexer::TokenType::Module)) {
    return attachAttributes(parseModuleDecl());
  }

  // 外部声明块
  if (match(lexer::TokenType::Extern)) {
    return attachAttributes(parseExternDecl());
  }

  // 检查是否是 public import 或普通 import
  ParserState state = saveState();
  std::string importSpecifiers = "";
  if (match(lexer::TokenType::Public)) {
    importSpecifiers = "public";
  } else if (match(lexer::TokenType::Private)) {
    importSpecifiers = "private";
  } else if (match(lexer::TokenType::Protected)) {
    importSpecifiers = "protected";
  } else if (match(lexer::TokenType::Internal)) {
    importSpecifiers = "internal";
  }
  if (check(lexer::TokenType::Import)) {
    advance();
    return parseImportDecl(importSpecifiers);
  }
  // 不是 import，恢复状态，让后面的 parse 函数自己处理访问修饰符
  restoreState(state);

  // 直接尝试解析各种声明，用 saveState 和 restoreState
  // 注意：各 parseXxx 内部可能调用 error() 抛出异常，必须用 try-catch
  // 确保异常安全

  // 0. 尝试解析扩展声明（直接检查关键字，避免被其他声明干扰）
  if (check(lexer::TokenType::Extension)) {
    return attachAttributes(parseExtensionDecl());
  }

  // 0.5. 尝试解析函数声明（直接检查 func 关键字）
  if (check(lexer::TokenType::Func)) {
    return attachAttributes(parseFunctionDecl());
  }

  ParserState tryState = saveState();
  // -1. 尝试解析 Getter 声明
  try {
    auto getterDecl = parseGetterDecl();
    if (getterDecl) {
      return attachAttributes(std::move(getterDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  tryState = saveState();
  // -0.5. 尝试解析 Setter 声明
  try {
    auto setterDecl = parseSetterDecl();
    if (setterDecl) {
      return attachAttributes(std::move(setterDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  // 1. 尝试解析函数声明
  tryState = saveState();
  try {
    auto funcDecl = parseFunctionDecl();
    if (funcDecl) {
      return attachAttributes(std::move(funcDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  // 1.5. 尝试解析 concept 声明
  if (check(lexer::TokenType::Concept)) {
    return attachAttributes(parseConceptDecl());
  }

  // 1.6. 尝试解析 attribute 声明
  if (check(lexer::TokenType::Attribute)) {
    return attachAttributes(parseAttributeDecl());
  }

  // 2. 尝试解析命名空间声明
  tryState = saveState();
  try {
    auto namespaceDecl = parseNamespaceDecl();
    if (namespaceDecl) {
      return attachAttributes(std::move(namespaceDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  // 3. 尝试解析类声明
  tryState = saveState();
  try {
    auto classDecl = parseClassDecl();
    if (classDecl) {
      return attachAttributes(std::move(classDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  // 4. 尝试解析接口声明
  tryState = saveState();
  try {
    auto interfaceDecl = parseInterfaceDecl();
    if (interfaceDecl) {
      return attachAttributes(std::move(interfaceDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  // 5. 尝试解析结构体声明
  tryState = saveState();
  try {
    auto structDecl = parseStructDecl();
    if (structDecl) {
      return attachAttributes(std::move(structDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  // 5. 尝试解析枚举声明
  tryState = saveState();
  try {
    auto enumDecl = parseEnumDecl();
    if (enumDecl) {
      return attachAttributes(std::move(enumDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  // 6. 尝试解析类型别名声明
  tryState = saveState();
  try {
    auto usingDecl = parseTypeAliasDecl();
    if (usingDecl) {
      return attachAttributes(std::move(usingDecl));
    }
  } catch (...) {
  }
  restoreState(tryState);

  // 7. 尝试解析 const 声明（编译期常量，类似 constexpr）
  if (check(lexer::TokenType::Const)) {
    advance(); // 消费 Const

    std::string specifiers = "";
    if (check(lexer::TokenType::Public) || check(lexer::TokenType::Private) ||
        check(lexer::TokenType::Protected) ||
        check(lexer::TokenType::Internal)) {
      specifiers = currentToken->getValue();
      advance();
    }

    auto type = parseType();
    if (!type) {
      error("Expected type after const");
      return nullptr;
    }

    if (!check(lexer::TokenType::Identifier)) {
      error("Expected identifier in const declaration");
      return nullptr;
    }
    std::string name = currentToken->getValue();
    advance();

    std::unique_ptr<ast::Expression> initializer;
    if (match(lexer::TokenType::Assign)) {
      initializer = parseExpression();
    }

    expect(lexer::TokenType::Semicolon, "Expected ';' after const declaration");

    // const 声明创建编译期常量，isConst = true
    return attachAttributes(std::make_unique<ast::VariableDecl>(
        specifiers, false, ast::VariableKind::Let, std::move(type), name,
        std::move(initializer), true, false));
  }

  // 8. 尝试解析函数声明（类成员方法，无 func 关键字）
  if (check(lexer::TokenType::Public) || check(lexer::TokenType::Private) ||
      check(lexer::TokenType::Protected) || check(lexer::TokenType::Internal) ||
      check(lexer::TokenType::Static) || isTypeStart()) {
    // 尝试解析类成员方法
    ParserState methodState = saveState();
    try {
      auto funcDecl = parseFunctionDecl();
      if (funcDecl) {
        return attachAttributes(std::move(funcDecl));
      }
    } catch (...) {
    }
    restoreState(methodState);
  }

  // 9. 尝试解析元组解构声明或变量声明
  if (check(lexer::TokenType::Var) || check(lexer::TokenType::Let) ||
      check(lexer::TokenType::Late) || isTypeStart() ||
      check(lexer::TokenType::Public) || check(lexer::TokenType::Private) ||
      check(lexer::TokenType::Protected) || check(lexer::TokenType::Internal) ||
      check(lexer::TokenType::Static)) {

    ParserState tempState = saveState();
    std::string specifiers = "";

    // 支持多个修饰符组合（如 public static、static 等）
    while (true) {
      if (match(lexer::TokenType::Public)) {
        specifiers += (specifiers.empty() ? "" : " ") + std::string("public");
      } else if (match(lexer::TokenType::Private)) {
        specifiers += (specifiers.empty() ? "" : " ") + std::string("private");
      } else if (match(lexer::TokenType::Protected)) {
        specifiers +=
            (specifiers.empty() ? "" : " ") + std::string("protected");
      } else if (match(lexer::TokenType::Internal)) {
        specifiers += (specifiers.empty() ? "" : " ") + std::string("internal");
      } else if (match(lexer::TokenType::Static)) {
        specifiers += (specifiers.empty() ? "" : " ") + std::string("static");
      } else {
        break;
      }
    }

    bool isLate = match(lexer::TokenType::Late);

    ast::VariableKind kind = ast::VariableKind::Explicit;
    if (match(lexer::TokenType::Var)) {
      kind = ast::VariableKind::Var;
    } else if (match(lexer::TokenType::Let)) {
      kind = ast::VariableKind::Let;
    }

    // 检查是否是元组解构（接下来是 ( ）
    if (check(lexer::TokenType::LParen)) {
      // 尝试解析元组解构声明
      auto tupleDestructuringDecl =
          parseTupleDestructuringDecl(specifiers, isLate, kind);
      if (tupleDestructuringDecl) {
        return attachAttributes(std::move(tupleDestructuringDecl));
      }
    }

    // 不是元组解构，恢复状态，尝试解析变量声明
    restoreState(tempState);
    auto varDecl = tryParseVariableDecl();
    if (varDecl) {
      return attachAttributes(std::move(varDecl));
    }

    // tryParse 失败，直接调用 parseVariableDecl 来解析，这样能正确解析 {} 语法
    return attachAttributes(parseVariableDecl());
  }

  error("Expected declaration");
  return nullptr;
}

// 解析模块声明
std::unique_ptr<ast::Declaration> Parser::parseModuleDecl() {
  std::vector<std::string> modulePath;

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected module name");
    return nullptr;
  }
  modulePath.push_back(currentToken->getValue());
  advance();

  while (match(lexer::TokenType::Dot)) {
    if (!check(lexer::TokenType::Identifier)) {
      error("Expected identifier after '.'");
      return nullptr;
    }
    modulePath.push_back(currentToken->getValue());
    advance();
  }

  expect(lexer::TokenType::Semicolon, "Expected ';' after module declaration");

  return std::make_unique<ast::ModuleDecl>(std::move(modulePath));
}

// 解析导入声明
std::unique_ptr<ast::Declaration>
Parser::parseImportDecl(const std::string &specifiers) {
  std::vector<std::string> modulePath;
  std::string alias;

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected module name");
    return nullptr;
  }
  modulePath.push_back(currentToken->getValue());
  advance();

  while (match(lexer::TokenType::Dot)) {
    if (!check(lexer::TokenType::Identifier)) {
      error("Expected identifier after '.'");
      return nullptr;
    }
    modulePath.push_back(currentToken->getValue());
    advance();
  }

  if (match(lexer::TokenType::As)) {
    if (!check(lexer::TokenType::Identifier)) {
      error("Expected alias after 'as'");
      return nullptr;
    }
    alias = currentToken->getValue();
    advance();
  }

  expect(lexer::TokenType::Semicolon, "Expected ';' after import declaration");

  return std::make_unique<ast::ImportDecl>(specifiers, std::move(modulePath),
                                           std::move(alias));
}

// 解析元组解构声明
std::unique_ptr<ast::TupleDestructuringDecl>
Parser::parseTupleDestructuringDecl(const std::string &specifiers, bool isLate,
                                    ast::VariableKind kind) {
  if (!match(lexer::TokenType::LParen)) {
    error("Expected '(' for tuple destructuring");
    return nullptr;
  }
  std::vector<std::string> names;
  if (!check(lexer::TokenType::RParen)) {
    do {
      if (!check(lexer::TokenType::Identifier)) {
        error("Expected identifier in tuple destructuring");
        return nullptr;
      }
      names.push_back(currentToken->getValue());
      advance();
    } while (match(lexer::TokenType::Comma));
  }
  if (!match(lexer::TokenType::RParen)) {
    error("Expected ')' after tuple destructuring names");
    return nullptr;
  }

  if (names.size() < 2) {
    error("Tuple destructuring must have at least 2 names");
    return nullptr;
  }

  std::unique_ptr<ast::Expression> initializer;
  if (match(lexer::TokenType::Assign)) {
    initializer = parseExpression();
  } else {
    error("Tuple destructuring declaration must have an initializer");
    return nullptr;
  }

  if (!match(lexer::TokenType::Semicolon)) {
    error("Expected ';' after tuple destructuring declaration");
    return nullptr;
  }

  return std::make_unique<ast::TupleDestructuringDecl>(
      specifiers, isLate, kind, std::move(names), std::move(initializer));
}

// 解析变量声明
std::unique_ptr<ast::VariableDecl> Parser::parseVariableDecl() {
  std::string specifiers = "";

  // 支持多个修饰符组合（如 public static）
  while (true) {
    if (match(lexer::TokenType::Public)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("public");
      // 处理可能的冒号
      if (match(lexer::TokenType::Colon)) {
        // 冒号已经被消费，继续解析
      }
    } else if (match(lexer::TokenType::Private)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("private");
      // 处理可能的冒号
      if (match(lexer::TokenType::Colon)) {
        // 冒号已经被消费，继续解析
      }
    } else if (match(lexer::TokenType::Protected)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("protected");
      // 处理可能的冒号
      if (match(lexer::TokenType::Colon)) {
        // 冒号已经被消费，继续解析
      }
    } else if (match(lexer::TokenType::Internal)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("internal");
      // 处理可能的冒号
      if (match(lexer::TokenType::Colon)) {
        // 冒号已经被消费，继续解析
      }
    } else if (match(lexer::TokenType::Static)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("static");
    } else {
      break;
    }
  }

  bool isLate = match(lexer::TokenType::Late);

  ast::VariableKind kind = ast::VariableKind::Explicit;

  if (match(lexer::TokenType::Var)) {
    kind = ast::VariableKind::Var;
  } else if (match(lexer::TokenType::Let)) {
    kind = ast::VariableKind::Let;
  }

  // 检查是否是元组解构（接下来是 ( ）
  if (check(lexer::TokenType::LParen)) {
    // 元组解构声明的解析会在 parseDeclaration 中处理
    error("Tuple destructuring should be handled in parseDeclaration");
    return nullptr;
  }

  std::unique_ptr<ast::Node> type;
  // 只有 Explicit 类型（没有 let/var）才需要显式类型
  if (kind == ast::VariableKind::Explicit) {
    if (auto parsedType = parseType()) {
      type = std::move(parsedType);
    }
  }

  if (!check(lexer::TokenType::Identifier)) {
    return nullptr;
  }
  std::string name = currentToken->getValue();
  advance();

  std::unique_ptr<ast::Expression> initializer;
  if (match(lexer::TokenType::Assign)) {
    initializer = parseExpression();
  } else if (check(lexer::TokenType::LBrace)) {
    if (type) {
      auto *typePtr = static_cast<ast::Type *>(type.get());
      initializer = parseStructInit(typePtr->clone());
    } else {
      initializer = parseStructInit();
    }
  }

  expect(lexer::TokenType::Semicolon,
         "Expected ';' after variable declaration");

  // 检查是否是静态变量
  bool isStatic = specifiers.find("static") != std::string::npos;

  return std::make_unique<ast::VariableDecl>(
      specifiers, isLate, kind, std::move(type), name, std::move(initializer),
      false, isStatic);
}

std::unique_ptr<ast::VariableDecl> Parser::parseVariableDeclForFor() {
  std::string specifiers = "";

  while (true) {
    if (match(lexer::TokenType::Public)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("public");
      if (match(lexer::TokenType::Colon)) {
      }
    } else if (match(lexer::TokenType::Private)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("private");
      if (match(lexer::TokenType::Colon)) {
      }
    } else if (match(lexer::TokenType::Protected)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("protected");
      if (match(lexer::TokenType::Colon)) {
      }
    } else if (match(lexer::TokenType::Internal)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("internal");
      if (match(lexer::TokenType::Colon)) {
      }
    } else if (match(lexer::TokenType::Static)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("static");
    } else {
      break;
    }
  }

  bool isLate = match(lexer::TokenType::Late);

  ast::VariableKind kind = ast::VariableKind::Explicit;

  if (match(lexer::TokenType::Var)) {
    kind = ast::VariableKind::Var;
  } else if (match(lexer::TokenType::Let)) {
    kind = ast::VariableKind::Let;
  }

  std::unique_ptr<ast::Node> type;
  if (kind == ast::VariableKind::Explicit) {
    if (auto parsedType = parseType()) {
      type = std::move(parsedType);
    }
  }

  if (!check(lexer::TokenType::Identifier)) {
    return nullptr;
  }
  std::string name = currentToken->getValue();
  advance();

  std::unique_ptr<ast::Expression> initializer;
  if (match(lexer::TokenType::Assign)) {
    initializer = parseExpression();
  } else if (check(lexer::TokenType::LBrace)) {
    if (type) {
      auto *typePtr = static_cast<ast::Type *>(type.get());
      initializer = parseStructInit(typePtr->clone());
    } else {
      initializer = parseStructInit();
    }
  }

  bool isStatic = specifiers.find("static") != std::string::npos;

  return std::make_unique<ast::VariableDecl>(
      specifiers, isLate, kind, std::move(type), name, std::move(initializer),
      false, isStatic);
}

// 尝试解析变量声明（失败时返回 nullptr，不抛异常）
std::unique_ptr<ast::VariableDecl> Parser::tryParseVariableDecl() {
  ParserState state = saveState();
  try {
    std::string specifiers = "";

    // 支持多个修饰符组合（如 public static、static 等）
    while (true) {
      if (match(lexer::TokenType::Public)) {
        specifiers += (specifiers.empty() ? "" : " ") + std::string("public");
        // 处理可能的冒号
        if (match(lexer::TokenType::Colon)) {
          // 冒号已经被消费，继续解析
        }
      } else if (match(lexer::TokenType::Private)) {
        specifiers += (specifiers.empty() ? "" : " ") + std::string("private");
        // 处理可能的冒号
        if (match(lexer::TokenType::Colon)) {
          // 冒号已经被消费，继续解析
        }
      } else if (match(lexer::TokenType::Protected)) {
        specifiers +=
            (specifiers.empty() ? "" : " ") + std::string("protected");
        // 处理可能的冒号
        if (match(lexer::TokenType::Colon)) {
          // 冒号已经被消费，继续解析
        }
      } else if (match(lexer::TokenType::Internal)) {
        specifiers += (specifiers.empty() ? "" : " ") + std::string("internal");
        // 处理可能的冒号
        if (match(lexer::TokenType::Colon)) {
          // 冒号已经被消费，继续解析
        }
      } else if (match(lexer::TokenType::Static)) {
        specifiers += (specifiers.empty() ? "" : " ") + std::string("static");
      } else {
        break;
      }
    }

    bool isLate = match(lexer::TokenType::Late);

    ast::VariableKind kind = ast::VariableKind::Explicit;

    if (match(lexer::TokenType::Var)) {
      kind = ast::VariableKind::Var;
    } else if (match(lexer::TokenType::Let)) {
      kind = ast::VariableKind::Let;
    }

    // 检查是否是元组解构（接下来是 ( ）
    if (check(lexer::TokenType::LParen)) {
      restoreState(state);
      return nullptr;
    }

    std::unique_ptr<ast::Node> type;
    // 只有 Explicit 类型（没有 let/var）才需要显式类型
    if (kind == ast::VariableKind::Explicit) {
      if (auto parsedType = parseType()) {
        type = std::move(parsedType);
      }
    }

    if (!check(lexer::TokenType::Identifier)) {
      restoreState(state);
      return nullptr;
    }
    std::string name = currentToken->getValue();
    advance();

    std::unique_ptr<ast::Expression> initializer;
    if (match(lexer::TokenType::Assign)) {
      initializer = parseExpression();
    } else if (check(lexer::TokenType::LBrace)) {
      if (type) {
        auto *typePtr = static_cast<ast::Type *>(type.get());
        initializer = parseStructInit(typePtr->clone());
      } else {
        initializer = parseStructInit();
      }
    }

    if (!check(lexer::TokenType::Semicolon)) {
      restoreState(state);
      return nullptr;
    }
    advance();

    // 检查是否是静态变量
    bool isStatic = specifiers.find("static") != std::string::npos;

    return std::make_unique<ast::VariableDecl>(
        specifiers, isLate, kind, std::move(type), name, std::move(initializer),
        false, isStatic);
  } catch (...) {
    restoreState(state);
    return nullptr;
  }
}

// 解析命名空间声明
std::unique_ptr<ast::NamespaceDecl> Parser::parseNamespaceDecl() {
  if (!match(lexer::TokenType::Namespace)) {
    return nullptr;
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected namespace name");
    return nullptr;
  }
  std::string name = currentToken->getValue();
  advance();

  if (!match(lexer::TokenType::LBrace)) {
    error("Expected '{' after namespace name");
    return nullptr;
  }

  std::vector<std::unique_ptr<ast::Node>> members;

  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    if (auto decl = parseDeclaration()) {
      members.push_back(std::move(decl));
    } else {
      advance();
    }
  }

  if (!match(lexer::TokenType::RBrace)) {
    error("Expected '}' to close namespace");
    return nullptr;
  }

  return std::make_unique<ast::NamespaceDecl>(name, std::move(members));
}

// 解析函数声明
std::unique_ptr<ast::FunctionDecl> Parser::parseFunctionDecl() {
  std::string specifiers = "";
  bool isImplicitOperator = false;
  std::unique_ptr<ast::Node> targetType;
  std::unique_ptr<ast::Node> returnType;

  // 首先保存当前状态，检查是否有访问修饰符和implicit operator
  ParserState checkAll = saveState();

  // 先尝试解析可能的访问修饰符（可以有多个：public static 等）
  std::string tempSpecifiers = "";
  while (true) {
    if (match(lexer::TokenType::Public)) {
      tempSpecifiers += "public ";
    } else if (match(lexer::TokenType::Private)) {
      tempSpecifiers += "private ";
    } else if (match(lexer::TokenType::Protected)) {
      tempSpecifiers += "protected ";
    } else if (match(lexer::TokenType::Internal)) {
      tempSpecifiers += "internal ";
    } else if (match(lexer::TokenType::Static)) {
      tempSpecifiers += "static ";
    } else if (match(lexer::TokenType::Virtual)) {
      tempSpecifiers += "virtual ";
    } else if (match(lexer::TokenType::Abstract)) {
      tempSpecifiers += "abstract ";
    } else {
      break;
    }
  }
  if (!tempSpecifiers.empty() && tempSpecifiers.back() == ' ') {
    tempSpecifiers.pop_back();
  }

  // 情况0: implicit operator
  if (match(lexer::TokenType::Implicit)) {
    if (match(lexer::TokenType::Operator)) {
      ParserState typeState = saveState();
      auto tempTarget = parseType();
      if (tempTarget && check(lexer::TokenType::LParen)) {
        isImplicitOperator = true;
      }
      restoreState(typeState);
    }
  }

  restoreState(checkAll);

  // 现在根据检查结果，消费实际的token（支持多个修饰符组合）
  while (true) {
    if (match(lexer::TokenType::Public)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("public");
    } else if (match(lexer::TokenType::Private)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("private");
    } else if (match(lexer::TokenType::Protected)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("protected");
    } else if (match(lexer::TokenType::Internal)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("internal");
    } else if (match(lexer::TokenType::Static)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("static");
    } else if (match(lexer::TokenType::Virtual)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("virtual");
    } else if (match(lexer::TokenType::Abstract)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("abstract");
    } else {
      break;
    }
  }

  std::string name;
  bool hasFunc = false;

  if (isImplicitOperator) {
    match(lexer::TokenType::Implicit);
    match(lexer::TokenType::Operator);
    targetType = parseType();
    if (!targetType) {
      error("Expected target type after 'operator'");
      return nullptr;
    }
    name = "implicit_operator_" + targetType->toString();
    hasFunc = true;
  }
  // 正常流程
  else {
    // 情况1: 带 func 关键字的函数
    if (match(lexer::TokenType::Func)) {
      hasFunc = true;
      // 检查是否是运算符重载
      if (match(lexer::TokenType::Operator)) {
        // 解析运算符
        if (check(lexer::TokenType::Plus)) {
          name = "operator+";
          advance();
        } else if (check(lexer::TokenType::Minus)) {
          name = "operator-";
          advance();
        } else if (check(lexer::TokenType::Multiply)) {
          name = "operator*";
          advance();
        } else if (check(lexer::TokenType::Divide)) {
          name = "operator/";
          advance();
        } else if (check(lexer::TokenType::Modulus)) {
          name = "operator%";
          advance();
        } else if (check(lexer::TokenType::Power)) {
          name = "operator**";
          advance();
        } else if (check(lexer::TokenType::Eq)) {
          name = "operator==";
          advance();
        } else if (check(lexer::TokenType::Ne)) {
          name = "operator!=";
          advance();
        } else if (check(lexer::TokenType::Lt)) {
          name = "operator<";
          advance();
        } else if (check(lexer::TokenType::Le)) {
          name = "operator<=";
          advance();
        } else if (check(lexer::TokenType::Gt)) {
          name = "operator>";
          advance();
        } else if (check(lexer::TokenType::Ge)) {
          name = "operator>=";
          advance();
        } else if (check(lexer::TokenType::LBracket)) {
          name = "operator[]";
          advance();
          expect(lexer::TokenType::RBracket,
                 "Expected ']' after '[' in operator[]");
        } else if (check(lexer::TokenType::LParen)) {
          name = "operator()";
          advance();
          expect(lexer::TokenType::RParen,
                 "Expected ')' after '(' in operator()");
        } else if (check(lexer::TokenType::AddAssign)) {
          name = "operator+=";
          advance();
        } else if (check(lexer::TokenType::SubAssign)) {
          name = "operator-=";
          advance();
        } else if (check(lexer::TokenType::MulAssign)) {
          name = "operator*=";
          advance();
        } else if (check(lexer::TokenType::DivAssign)) {
          name = "operator/=";
          advance();
        } else if (check(lexer::TokenType::ModAssign)) {
          name = "operator%=";
          advance();
        } else if (check(lexer::TokenType::And)) {
          name = "operator&";
          advance();
        } else if (check(lexer::TokenType::Bar)) {
          name = "operator|";
          advance();
        } else if (check(lexer::TokenType::Tilde)) {
          name = "operator~";
          advance();
        } else if (check(lexer::TokenType::Not)) {
          name = "operator!";
          advance();
        } else if (check(lexer::TokenType::LogicAnd)) {
          name = "operator&&";
          advance();
        } else if (check(lexer::TokenType::LogicOr)) {
          name = "operator||";
          advance();
        } else if (check(lexer::TokenType::Shl)) {
          name = "operator<<";
          advance();
        } else if (check(lexer::TokenType::Shr)) {
          name = "operator>>";
          advance();
        } else {
          error("Expected operator symbol after 'operator'");
          return nullptr;
        }
      } else {
        // 普通函数
        if (!check(lexer::TokenType::Identifier)) {
          error("Expected function name");
          return nullptr;
        }
        name = currentToken->getValue();
        advance();
      }
    }
    // 情况2: 析构函数 ~Name
    else if (check(lexer::TokenType::Tilde)) {
      advance();
      if (!check(lexer::TokenType::Identifier)) {
        error("Expected identifier after '~' for destructor");
        return nullptr;
      }
      name = "~" + currentToken->getValue();
      advance();
    }
    // 情况2.5: 操作符重载（无 func 关键字，直接 operator）
    else if (check(lexer::TokenType::Operator)) {
      advance();
      if (check(lexer::TokenType::Plus)) {
        name = "operator+";
        advance();
      } else if (check(lexer::TokenType::Minus)) {
        name = "operator-";
        advance();
      } else if (check(lexer::TokenType::Multiply)) {
        name = "operator*";
        advance();
      } else if (check(lexer::TokenType::Divide)) {
        name = "operator/";
        advance();
      } else if (check(lexer::TokenType::Modulus)) {
        name = "operator%";
        advance();
      } else if (check(lexer::TokenType::Power)) {
        name = "operator**";
        advance();
      } else if (check(lexer::TokenType::Eq)) {
        name = "operator==";
        advance();
      } else if (check(lexer::TokenType::Ne)) {
        name = "operator!=";
        advance();
      } else if (check(lexer::TokenType::Lt)) {
        name = "operator<";
        advance();
      } else if (check(lexer::TokenType::Le)) {
        name = "operator<=";
        advance();
      } else if (check(lexer::TokenType::Gt)) {
        name = "operator>";
        advance();
      } else if (check(lexer::TokenType::Ge)) {
        name = "operator>=";
        advance();
      } else if (check(lexer::TokenType::LBracket)) {
        name = "operator[]";
        advance();
        expect(lexer::TokenType::RBracket,
               "Expected ']' after '[' in operator[]");
      } else if (check(lexer::TokenType::LParen)) {
        name = "operator()";
        advance();
        expect(lexer::TokenType::RParen,
               "Expected ')' after '(' in operator()");
      } else if (check(lexer::TokenType::AddAssign)) {
        name = "operator+=";
        advance();
      } else if (check(lexer::TokenType::SubAssign)) {
        name = "operator-=";
        advance();
      } else if (check(lexer::TokenType::MulAssign)) {
        name = "operator*=";
        advance();
      } else if (check(lexer::TokenType::DivAssign)) {
        name = "operator/=";
        advance();
      } else if (check(lexer::TokenType::ModAssign)) {
        name = "operator%=";
        advance();
      } else if (check(lexer::TokenType::And)) {
        name = "operator&";
        advance();
      } else if (check(lexer::TokenType::Bar)) {
        name = "operator|";
        advance();
      } else if (check(lexer::TokenType::Tilde)) {
        name = "operator~";
        advance();
      } else if (check(lexer::TokenType::Not)) {
        name = "operator!";
        advance();
      } else if (check(lexer::TokenType::LogicAnd)) {
        name = "operator&&";
        advance();
      } else if (check(lexer::TokenType::LogicOr)) {
        name = "operator||";
        advance();
      } else if (check(lexer::TokenType::Shl)) {
        name = "operator<<";
        advance();
      } else if (check(lexer::TokenType::Shr)) {
        name = "operator>>";
        advance();
      } else {
        error("Expected operator symbol after 'operator'");
        return nullptr;
      }
    }
    // 情况3: 构造函数 Name（无 func，无 ~，但跟着 (）
    else if (check(lexer::TokenType::Identifier)) {
      // 我们需要前瞻一个 token 看看是不是 '('，如果是，那就是构造函数
      ParserState state = saveState();
      name = currentToken->getValue();
      advance();
      if (check(lexer::TokenType::LParen)) {
        // 是构造函数
      } else {
        // 不是，恢复状态
        restoreState(state);
        // 尝试解析类成员方法（无 func 关键字，有返回类型）
        ParserState methodState = saveState();
        returnType = parseType();
        // 方法名可以是标识符，也可以是某些关键字（如 get、set）
        if (returnType &&
            (check(lexer::TokenType::Identifier) ||
             check(lexer::TokenType::Get) || check(lexer::TokenType::Set))) {
          // 有返回类型，这是一个类成员方法
          name = currentToken->getValue();
          advance();
        } else {
          // 不是类成员方法，恢复状态
          restoreState(methodState);
          return nullptr;
        }
      }
    }
    // 情况4: 类成员方法（有返回类型，无 func 关键字）
    else if (isTypeStart()) {
      // 尝试解析类成员方法
      ParserState methodState = saveState();
      returnType = parseType();
      // 方法名可以是标识符，也可以是某些关键字（如 get、set）
      if (returnType &&
          (check(lexer::TokenType::Identifier) ||
           check(lexer::TokenType::Get) || check(lexer::TokenType::Set))) {
        // 有返回类型，检查后面是否有左括号
        name = currentToken->getValue();
        advance();
        if (check(lexer::TokenType::LParen)) {
          // 是类成员方法
        } else {
          // 不是类成员方法，恢复状态
          restoreState(methodState);
          return nullptr;
        }
      } else {
        // 不是类成员方法，恢复状态
        restoreState(methodState);
        return nullptr;
      }
    } else {
      // 都不是
      return nullptr;
    }
  }

  std::vector<std::unique_ptr<ast::Node>> templateParams;
  if (match(lexer::TokenType::Lt)) {
    auto rawTemplateParams = parseTemplateParameters();
    for (auto &param : rawTemplateParams) {
      templateParams.push_back(std::unique_ptr<ast::Node>(param.release()));
    }
    expect(lexer::TokenType::Gt, "Expected '>' after template parameters");
  }

  expect(lexer::TokenType::LParen, "Expected '(' after function name");
  auto params = parseParameterList();
  expect(lexer::TokenType::RParen, "Expected ')' after parameters");

  // 检查函数后缀 ! 表示不可变方法（必须在 -> 之前，语法为 func foo()! -> Type）
  bool isImmutable = false;
  if (!isImplicitOperator && match(lexer::TokenType::Not)) {
    isImmutable = true;
  }

  if (isImplicitOperator) {
    returnType = std::move(targetType);
  } else if (!returnType && match(lexer::TokenType::Arrow)) {
    returnType = parseType();
  }

  std::unique_ptr<ast::Node> whereClause;
  if (match(lexer::TokenType::Where)) {
    whereClause = parseWhereClause();
  }

  std::unique_ptr<ast::Node> requiresClause;
  if (match(lexer::TokenType::Requires)) {
    requiresClause = parseRequiresClause();
  }

  std::unique_ptr<ast::Node> body;
  std::unique_ptr<ast::Expression> arrowExpr;

  // 解析 super() 构造函数调用
  std::unique_ptr<ast::Expression> superCall;
  if (match(lexer::TokenType::Colon)) {
    if (match(lexer::TokenType::Super)) {
      expect(lexer::TokenType::LParen, "Expected '(' after 'super'");
      superCall = parseExpression();
      expect(lexer::TokenType::RParen, "Expected ')' after super arguments");
    }
  }

  if (match(lexer::TokenType::FatArrow)) {
    arrowExpr = parseExpression();
    expect(lexer::TokenType::Semicolon, "Expected ';' after arrow expression");
  } else if (check(lexer::TokenType::LBrace)) {
    body = parseCompoundStmt();
  } else {
    expect(lexer::TokenType::Semicolon,
           "Expected ';' after function declaration");
  }

  // 检查参数列表中是否有可变参数
  bool isVariadic = false;
  for (const auto &param : params) {
    if (auto *p = dynamic_cast<ast::Parameter *>(param.get())) {
      if (p->isVariadic) {
        isVariadic = true;
        break;
      }
    }
  }

  // 检查是否是静态函数
  bool isStatic = specifiers.find("static") != std::string::npos;

  return std::make_unique<ast::FunctionDecl>(
      specifiers, name, std::move(templateParams), std::move(params),
      std::move(returnType), std::move(whereClause), std::move(requiresClause),
      std::move(body), std::move(arrowExpr), isImmutable, std::move(superCall),
      isVariadic, isStatic);
}

// 解析类声明
std::unique_ptr<ast::ClassDecl> Parser::parseClassDecl() {
  std::string specifiers = "";

  // 支持多个修饰符组合（如 public abstract、abstract 等）
  while (true) {
    if (match(lexer::TokenType::Public)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("public");
    } else if (match(lexer::TokenType::Private)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("private");
    } else if (match(lexer::TokenType::Protected)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("protected");
    } else if (match(lexer::TokenType::Internal)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("internal");
    } else if (match(lexer::TokenType::Abstract)) {
      specifiers += (specifiers.empty() ? "" : " ") + std::string("abstract");
    } else {
      break;
    }
  }

  // 检查并消费 Class 关键字
  if (!match(lexer::TokenType::Class)) {
    return nullptr; // 不是类声明，返回 nullptr
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected class name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  // 解析模板参数
  std::vector<std::unique_ptr<ast::Node>> templateParams;
  if (match(lexer::TokenType::Lt)) {
    auto rawTemplateParams = parseTemplateParameters();
    for (auto &param : rawTemplateParams) {
      templateParams.push_back(std::unique_ptr<ast::Node>(param.release()));
    }
    if (!match(lexer::TokenType::Gt)) {
      error("Expected '>' after template parameters");
    }
  }

  std::string baseClass;
  std::vector<std::string> baseClasses;
  std::vector<std::string> interfaces;
  if (match(lexer::TokenType::Colon)) {
    // 解析继承列表（基类和接口）
    do {
      if (!check(lexer::TokenType::Identifier)) {
        error("Expected base class or interface name");
        return nullptr;
      }

      std::string name = currentToken->getValue();
      advance();

      // 检查是否是第一个基类
      if (baseClass.empty()) {
        baseClass = name;
        baseClasses.push_back(name);
      } else {
        // 其余的都作为基类（多继承）
        baseClasses.push_back(name);
      }
    } while (match(lexer::TokenType::Comma));
  }

  expect(lexer::TokenType::LBrace, "Expected '{' after class declaration");

  std::vector<std::unique_ptr<ast::Node>> members;
  std::string currentAccessSpecifier;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    // 检查并跳过访问修饰符（如 public:, private:, protected:）
    if (check(lexer::TokenType::Public) || check(lexer::TokenType::Private) ||
        check(lexer::TokenType::Protected) ||
        check(lexer::TokenType::Internal)) {
      // 保存状态
      ParserState state = saveState();
      auto accessToken = currentToken;
      // 消费访问修饰符
      advance();
      // 检查是否有冒号
      if (check(lexer::TokenType::Colon)) {
        // 消费冒号
        advance();
        // 这是访问修饰符声明，记录当前访问级别并跳过它
        if (accessToken) {
          currentAccessSpecifier = accessToken->getValue();
        }
        continue;
      }
      // 如果没有冒号，说明这是成员的访问修饰符，恢复状态
      restoreState(state);
    }

    auto decl = parseDeclaration();
    if (decl) {
      if (!currentAccessSpecifier.empty()) {
        if (auto *funcDecl = dynamic_cast<ast::FunctionDecl *>(decl.get())) {
          if (funcDecl->specifiers.empty()) {
            funcDecl->specifiers = currentAccessSpecifier;
          }
        } else if (auto *varDecl =
                       dynamic_cast<ast::VariableDecl *>(decl.get())) {
          if (varDecl->specifiers.empty()) {
            varDecl->specifiers = currentAccessSpecifier;
          }
        } else if (auto *getterDecl =
                       dynamic_cast<ast::GetterDecl *>(decl.get())) {
          if (getterDecl->specifiers.empty()) {
            getterDecl->specifiers = currentAccessSpecifier;
          }
        } else if (auto *setterDecl =
                       dynamic_cast<ast::SetterDecl *>(decl.get())) {
          if (setterDecl->specifiers.empty()) {
            setterDecl->specifiers = currentAccessSpecifier;
          }
        }
      }
      members.push_back(std::move(decl));
    } else {
      // 如果解析失败，跳过当前token以避免无限循环
      advance();
    }
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after class body");
  match(lexer::TokenType::Semicolon);

  return std::make_unique<ast::ClassDecl>(
      specifiers, name, std::move(templateParams), baseClass,
      std::move(baseClasses), std::move(interfaces), std::move(members));
}

// 解析接口声明
std::unique_ptr<ast::InterfaceDecl> Parser::parseInterfaceDecl() {
  std::string specifiers = "";

  if (match(lexer::TokenType::Public)) {
    specifiers = "public";
  } else if (match(lexer::TokenType::Private)) {
    specifiers = "private";
  } else if (match(lexer::TokenType::Protected)) {
    specifiers = "protected";
  } else if (match(lexer::TokenType::Internal)) {
    specifiers = "internal";
  }

  // 检查并消费 Interface 关键字
  if (!match(lexer::TokenType::Interface)) {
    return nullptr; // 不是接口声明，返回 nullptr
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected interface name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  // 解析父接口列表
  std::vector<std::string> baseInterfaces;
  if (match(lexer::TokenType::Colon)) {
    // 解析继承列表（父接口）
    do {
      if (!check(lexer::TokenType::Identifier)) {
        error("Expected base interface name");
        return nullptr;
      }

      std::string baseName = currentToken->getValue();
      advance();
      baseInterfaces.push_back(baseName);
    } while (match(lexer::TokenType::Comma));
  }

  expect(lexer::TokenType::LBrace, "Expected '{' after interface declaration");

  std::vector<std::unique_ptr<ast::Node>> members;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    // 接口成员只能是方法声明（没有函数体）
    auto member = parseDeclaration();
    if (member) {
      members.push_back(std::move(member));
    }
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after interface body");

  return std::make_unique<ast::InterfaceDecl>(
      specifiers, name, std::move(baseInterfaces), std::move(members));
}

// 解析 concept 声明
std::unique_ptr<ast::ConceptDecl> Parser::parseConceptDecl() {
  // 消费 concept 关键字
  if (!match(lexer::TokenType::Concept)) {
    return nullptr;
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected concept name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  // 解析模板参数 <T, U, ...>
  std::vector<std::unique_ptr<ast::Node>> templateParams;
  if (match(lexer::TokenType::Lt)) {
    do {
      if (!check(lexer::TokenType::Identifier)) {
        error("Expected template parameter name");
        return nullptr;
      }
      std::string paramName = currentToken->getValue();
      advance();
      templateParams.push_back(
          std::make_unique<ast::TemplateParameter>(paramName));
    } while (match(lexer::TokenType::Comma));

    if (!check(lexer::TokenType::Gt)) {
      error("Expected '>' after template parameters");
      return nullptr;
    }
    advance(); // 消费 '>'
  }

  // 解析约束（可选）
  std::vector<std::unique_ptr<ast::Node>> constraints;
  if (match(lexer::TokenType::Where)) {
    // 简化处理：跳过 where 子句内容直到 { 或 ;
    while (!check(lexer::TokenType::LBrace) &&
           !check(lexer::TokenType::Semicolon) &&
           !check(lexer::TokenType::EndOfFile)) {
      advance();
    }
  }

  // 解析 body
  if (match(lexer::TokenType::LBrace)) {
    // 有 body 的 concept
    while (!check(lexer::TokenType::RBrace) &&
           !check(lexer::TokenType::EndOfFile)) {
      advance(); // 简化处理，跳过 body 内容
    }
    expect(lexer::TokenType::RBrace, "Expected '}' after concept body");
  } else {
    // 无 body 的 concept（只有 where 子句）
    expect(lexer::TokenType::Semicolon,
           "Expected ';' or '{' after concept declaration");
  }

  return std::make_unique<ast::ConceptDecl>(name, std::move(templateParams),
                                            std::move(constraints));
}

// 解析 attribute 声明
std::unique_ptr<ast::AttributeDecl> Parser::parseAttributeDecl() {
  if (!match(lexer::TokenType::Attribute)) {
    return nullptr;
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected attribute name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  auto attrDecl = std::make_unique<ast::AttributeDecl>(name);

  if (!match(lexer::TokenType::LBrace)) {
    error("Expected '{' after attribute name");
    return nullptr;
  }

  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    if (!check(lexer::TokenType::Identifier)) {
      error("Expected field name in attribute");
      return nullptr;
    }

    std::string fieldName = currentToken->getValue();
    advance();

    std::unique_ptr<ast::Type> fieldType;
    if (match(lexer::TokenType::Colon)) {
      fieldType = parseType();
      if (!fieldType) {
        error("Expected type after ':'");
        return nullptr;
      }
    }

    std::unique_ptr<ast::Expression> defaultValue;
    if (match(lexer::TokenType::Assign)) {
      defaultValue = parseExpression();
      if (!defaultValue) {
        error("Expected default value after '='");
        return nullptr;
      }
    }

    auto field = std::make_unique<ast::AttributeField>(
        fieldName, std::move(fieldType), std::move(defaultValue));
    attrDecl->addField(std::move(field));

    if (!match(lexer::TokenType::Comma)) {
      break;
    }
  }

  if (!match(lexer::TokenType::RBrace)) {
    error("Expected '}' after attribute fields");
    return nullptr;
  }

  return attrDecl;
}

// 解析 attribute 应用
std::unique_ptr<ast::AttributeApplication> Parser::parseAttributeApplication() {
  if (!match(lexer::TokenType::LBracket)) {
    return nullptr;
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected attribute name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  auto attrApp = std::make_unique<ast::AttributeApplication>(name);

  // 解析参数列表
  if (match(lexer::TokenType::LParen)) {
    while (!check(lexer::TokenType::RParen) &&
           !check(lexer::TokenType::EndOfFile)) {
      std::string argName = "";
      ParserState state = saveState();

      // 检查是否是命名参数 (name = value)
      if (check(lexer::TokenType::Identifier)) {
        std::string potentialName = currentToken->getValue();
        advance();
        if (match(lexer::TokenType::Assign)) {
          argName = potentialName;
        } else {
          restoreState(state);
        }
      }

      auto value = parseExpression();
      if (!value) {
        error("Expected attribute argument value");
        return nullptr;
      }

      auto arg =
          std::make_unique<ast::AttributeArgument>(argName, std::move(value));
      attrApp->addArgument(std::move(arg));

      if (!match(lexer::TokenType::Comma)) {
        break;
      }
    }

    if (!match(lexer::TokenType::RParen)) {
      error("Expected ')' after attribute arguments");
      return nullptr;
    }
  }

  if (!match(lexer::TokenType::RBracket)) {
    error("Expected ']' after attribute");
    return nullptr;
  }

  return attrApp;
}

// 解析结构体声明
std::unique_ptr<ast::StructDecl> Parser::parseStructDecl() {
  std::string specifiers = "";

  if (match(lexer::TokenType::Public)) {
    specifiers = "public";
  } else if (match(lexer::TokenType::Private)) {
    specifiers = "private";
  } else if (match(lexer::TokenType::Protected)) {
    specifiers = "protected";
  } else if (match(lexer::TokenType::Internal)) {
    specifiers = "internal";
  }

  // 检查并消费 Struct 关键字
  if (!match(lexer::TokenType::Struct)) {
    return nullptr; // 不是结构体声明，返回 nullptr
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected struct name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  expect(lexer::TokenType::LBrace, "Expected '{' after struct declaration");

  std::vector<std::unique_ptr<ast::Node>> members;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    members.push_back(parseDeclaration());
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after struct body");

  return std::make_unique<ast::StructDecl>(specifiers, name,
                                           std::move(members));
}

// 解析枚举声明
std::unique_ptr<ast::EnumDecl> Parser::parseEnumDecl() {
  std::string specifiers = "";

  if (match(lexer::TokenType::Public)) {
    specifiers = "public";
  } else if (match(lexer::TokenType::Private)) {
    specifiers = "private";
  } else if (match(lexer::TokenType::Protected)) {
    specifiers = "protected";
  } else if (match(lexer::TokenType::Internal)) {
    specifiers = "internal";
  }

  // 检查并消费 Enum 关键字
  if (!match(lexer::TokenType::Enum)) {
    return nullptr; // 不是枚举声明，返回 nullptr
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected enum name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  expect(lexer::TokenType::LBrace, "Expected '{' after enum declaration");

  std::vector<std::unique_ptr<ast::EnumMember>> members;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    members.push_back(parseEnumMember());
    if (!match(lexer::TokenType::Comma)) {
      break;
    }
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after enum body");

  return std::make_unique<ast::EnumDecl>(specifiers, name, std::move(members));
}

// 解析枚举成员
std::unique_ptr<ast::EnumMember> Parser::parseEnumMember() {
  if (!check(lexer::TokenType::Identifier)) {
    error("Expected enum member name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  std::unique_ptr<ast::Expression> value;
  if (match(lexer::TokenType::Assign)) {
    value = parseExpression();
  }

  return std::make_unique<ast::EnumMember>(name, std::move(value));
}

// 解析扩展声明
std::unique_ptr<ast::Declaration> Parser::parseExtensionDecl() {
  // 检查并消费 Extension 关键字
  if (!match(lexer::TokenType::Extension)) {
    return nullptr;
  }

  // 解析被扩展的类型
  auto extendedType = parseType();
  if (!extendedType) {
    error("Expected type after 'extension'");
    return nullptr;
  }

  expect(lexer::TokenType::LBrace, "Expected '{' after extended type");

  std::vector<std::unique_ptr<ast::Node>> members;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    members.push_back(parseDeclaration());
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after extension body");

  return std::make_unique<ast::ExtensionDecl>(std::move(extendedType),
                                              std::move(members));
}

// 解析 Getter 声明
std::unique_ptr<ast::GetterDecl> Parser::parseGetterDecl() {
  // 先解析访问修饰符和 static（可能在 get 关键字之前）
  std::string specifiers = "";
  while (true) {
    if (check(lexer::TokenType::Public)) {
      specifiers += "public ";
      advance();
    } else if (check(lexer::TokenType::Private)) {
      specifiers += "private ";
      advance();
    } else if (check(lexer::TokenType::Protected)) {
      specifiers += "protected ";
      advance();
    } else if (check(lexer::TokenType::Internal)) {
      specifiers += "internal ";
      advance();
    } else if (check(lexer::TokenType::Static)) {
      specifiers += "static ";
      advance();
    } else {
      break;
    }
  }

  // 检查是否是 getter 声明（必须有 get 关键字）
  if (!check(lexer::TokenType::Get)) {
    return nullptr;
  }

  // 消费 Get 关键字
  advance();

  // Getter 名称必须存在；若后续不是标识符，则回退，不视为 GetterDecl
  if (!check(lexer::TokenType::Identifier)) {
    return nullptr;
  }
  std::string name = currentToken->getValue();
  advance();

  // Getter 必须带返回类型：通过 '->' 指定类型
  std::unique_ptr<ast::Node> returnType;
  if (!match(lexer::TokenType::Arrow)) {
    error("Expected '->' return type after getter name");
    return nullptr;
  }
  returnType = parseType();
  if (!returnType) {
    error("Expected return type after '->'");
    return nullptr;
  }

  // 解析函数体或箭头表达式
  std::unique_ptr<ast::Node> body;
  std::unique_ptr<ast::Expression> arrowExpr;

  if (match(lexer::TokenType::FatArrow)) {
    arrowExpr = parseExpression();
    expect(lexer::TokenType::Semicolon, "Expected ';' after arrow expression");
  } else if (check(lexer::TokenType::LBrace)) {
    body = parseCompoundStmt();
  } else {
    // GetterDecl 语法需要一个实现体
    error("Expected '{' after getter return type");
  }

  return std::make_unique<ast::GetterDecl>(
      specifiers, name, std::move(returnType), std::move(body),
      std::move(arrowExpr));
}

// 解析 Setter 声明
std::unique_ptr<ast::SetterDecl> Parser::parseSetterDecl() {
  // 先解析访问修饰符和 static（可能在 set 关键字之前）
  std::string specifiers = "";
  while (true) {
    if (check(lexer::TokenType::Public)) {
      specifiers += "public ";
      advance();
    } else if (check(lexer::TokenType::Private)) {
      specifiers += "private ";
      advance();
    } else if (check(lexer::TokenType::Protected)) {
      specifiers += "protected ";
      advance();
    } else if (check(lexer::TokenType::Internal)) {
      specifiers += "internal ";
      advance();
    } else if (check(lexer::TokenType::Static)) {
      specifiers += "static ";
      advance();
    } else {
      break;
    }
  }

  // 检查是否是 setter 声明（必须有 set 关键字）
  if (!check(lexer::TokenType::Set)) {
    return nullptr;
  }

  // 消费 Set 关键字
  advance();

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected setter name");
    return nullptr;
  }
  std::string name = currentToken->getValue();
  advance();

  // 解析参数
  expect(lexer::TokenType::LParen, "Expected '(' after setter name");

  auto param = parseParameter();
  if (!param) {
    error("Expected parameter for setter");
    return nullptr;
  }

  expect(lexer::TokenType::RParen, "Expected ')' after setter parameter");

  // 解析函数体或箭头表达式
  std::unique_ptr<ast::Node> body;
  std::unique_ptr<ast::Expression> arrowExpr;

  if (match(lexer::TokenType::FatArrow)) {
    arrowExpr = parseExpression();
    expect(lexer::TokenType::Semicolon, "Expected ';' after arrow expression");
  } else if (check(lexer::TokenType::LBrace)) {
    body = parseCompoundStmt();
  } else {
    expect(lexer::TokenType::Semicolon,
           "Expected ';' after setter declaration");
  }

  // 将 unique_ptr<Node> 转换为 unique_ptr<Parameter>
  auto parameter = std::unique_ptr<ast::Parameter>(
      static_cast<ast::Parameter *>(param.release()));

  return std::make_unique<ast::SetterDecl>(
      specifiers, name, std::move(parameter), std::move(body),
      std::move(arrowExpr));
}

// 解析类型别名声明
std::unique_ptr<ast::TypeAliasDecl> Parser::parseTypeAliasDecl() {
  std::string specifiers = "";

  if (match(lexer::TokenType::Public)) {
    specifiers = "public";
  } else if (match(lexer::TokenType::Private)) {
    specifiers = "private";
  } else if (match(lexer::TokenType::Protected)) {
    specifiers = "protected";
  } else if (match(lexer::TokenType::Internal)) {
    specifiers = "internal";
  }

  // 检查并消费 Using 关键字
  if (!match(lexer::TokenType::Using)) {
    return nullptr; // 不是类型别名声明，返回 nullptr
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected type alias name");
    return nullptr;
  }
  std::string name = currentToken->getValue();
  advance();

  expect(lexer::TokenType::Assign, "Expected '=' after type alias name");

  auto type = parseType();
  if (!type) {
    error("Expected type after '='");
    return nullptr;
  }

  // 检查是否有类型集合约束 (int | long | float | ...)
  bool isTypeSet = false;
  while (match(lexer::TokenType::Bar)) {
    isTypeSet = true;
    auto nextType = parseType();
    if (!nextType) {
      error("Expected type after '|'");
      return nullptr;
    }
    // 简化处理：忽略后续类型，只保留第一个类型
    // TODO: 实现完整的类型集合约束
  }

  expect(lexer::TokenType::Semicolon,
         "Expected ';' after type alias declaration");

  return std::make_unique<ast::TypeAliasDecl>(specifiers, name, std::move(type),
                                              isTypeSet);
}

// 解析语句
std::unique_ptr<ast::Statement> Parser::parseStatement() {
  // 解析属性列表
  std::vector<std::unique_ptr<ast::AttributeApplication>> attributes;
  while (check(lexer::TokenType::LBracket)) {
    // 检查是否是属性应用（后面跟着标识符，然后是 ] 或 (）
    ParserState state = saveState();
    advance(); // 消费 [
    if (check(lexer::TokenType::Identifier)) {
      std::string name = currentToken->getValue();
      advance();
      // 如果后面是 ] 或 (，则是属性应用
      if (check(lexer::TokenType::RBracket) ||
          check(lexer::TokenType::LParen)) {
        restoreState(state);
        auto attr = parseAttributeApplication();
        if (attr) {
          attributes.push_back(std::move(attr));
        }
        continue;
      }
    }
    // 不是属性应用，恢复状态，让表达式解析处理
    restoreState(state);
    break;
  }

  // 辅助函数：将属性附加到语句
  auto attachAttributes = [&attributes](std::unique_ptr<ast::Statement> stmt)
      -> std::unique_ptr<ast::Statement> {
    if (stmt && !attributes.empty()) {
      stmt->attributes = std::move(attributes);
    }
    return stmt;
  };

  if (check(lexer::TokenType::LBrace)) {
    return attachAttributes(parseCompoundStmt());
  } else if (match(lexer::TokenType::Semicolon)) {
    return nullptr;
  } else if (match(lexer::TokenType::If)) {
    return attachAttributes(parseIfStmt());
  } else if (match(lexer::TokenType::While)) {
    return attachAttributes(parseWhileStmt());
  } else if (match(lexer::TokenType::Return)) {
    return attachAttributes(parseReturnStmt());
  } else if (match(lexer::TokenType::Yield)) {
    return attachAttributes(parseYieldStmt());
  } else if (match(lexer::TokenType::For)) {
    return attachAttributes(parseForStmt());
  } else if (match(lexer::TokenType::Foreach)) {
    return attachAttributes(parseForeachStmt());
  } else if (match(lexer::TokenType::Comptime)) {
    return attachAttributes(parseComptimeStmt());
  } else if (match(lexer::TokenType::Break)) {
    expect(lexer::TokenType::Semicolon, "Expected ';' after break");
    return attachAttributes(std::make_unique<ast::BreakStmt>());
  } else if (match(lexer::TokenType::Continue)) {
    expect(lexer::TokenType::Semicolon, "Expected ';' after continue");
    return attachAttributes(std::make_unique<ast::ContinueStmt>());
  } else if (match(lexer::TokenType::Goto)) {
    if (!check(lexer::TokenType::Identifier)) {
      error("Expected label name after goto");
      return nullptr;
    }
    std::string label = currentToken->getValue();
    advance();
    expect(lexer::TokenType::Semicolon, "Expected ';' after goto label");
    return attachAttributes(std::make_unique<ast::GotoStmt>(std::move(label)));
  } else if (match(lexer::TokenType::Try)) {
    return attachAttributes(parseTryStmt());
  } else if (match(lexer::TokenType::Throw)) {
    return attachAttributes(parseThrowStmt());
  } else if (match(lexer::TokenType::Defer)) {
    return attachAttributes(parseDeferStmt());
  } else if (match(lexer::TokenType::Match)) {
    auto matchStmt = parseMatchStmt();
    match(lexer::TokenType::Semicolon); // 消费可选的分号
    return attachAttributes(std::move(matchStmt));
  } else if (check(lexer::TokenType::Const)) {
    advance(); // 消费 Const

    std::string specifiers = "";
    if (check(lexer::TokenType::Public) || check(lexer::TokenType::Private) ||
        check(lexer::TokenType::Protected) ||
        check(lexer::TokenType::Internal)) {
      specifiers = currentToken->getValue();
      advance();
    }

    auto type = parseType();
    if (!type) {
      error("Expected type after const");
      return nullptr;
    }

    if (!check(lexer::TokenType::Identifier)) {
      error("Expected identifier in const declaration");
      return nullptr;
    }
    std::string name = currentToken->getValue();
    advance();

    std::unique_ptr<ast::Expression> initializer;
    if (match(lexer::TokenType::Assign)) {
      initializer = parseExpression();
    }

    expect(lexer::TokenType::Semicolon, "Expected ';' after const declaration");

    auto varDecl = std::make_unique<ast::VariableDecl>(
        specifiers, false, ast::VariableKind::Let, std::move(type), name,
        std::move(initializer), true);
    auto stmt = std::make_unique<ast::VariableStmt>(std::move(varDecl));
    if (!attributes.empty()) {
      stmt->attributes = std::move(attributes);
    }
    return stmt;
  } else if (check(lexer::TokenType::Identifier)) {
    // 检查是否是标签（后面跟着冒号）
    auto state = saveState();
    std::string label = currentToken->getValue();
    advance();
    if (match(lexer::TokenType::Colon)) {
      return attachAttributes(
          std::make_unique<ast::LabelStmt>(std::move(label)));
    }
    // 不是标签，回滚并继续解析
    restoreState(state);
  }

  if (check(lexer::TokenType::Var) || check(lexer::TokenType::Let) ||
      check(lexer::TokenType::Late) || isTypeStart() ||
      check(lexer::TokenType::Static)) {
    // 先尝试解析 tuple 解构声明
    ParserState tempState = saveState();
    std::string specifiers = "";
    // 先消费可能的 static 修饰符
    if (match(lexer::TokenType::Static)) {
      specifiers = "static";
    }
    bool isLate = match(lexer::TokenType::Late);

    ast::VariableKind kind = ast::VariableKind::Explicit;
    if (match(lexer::TokenType::Var)) {
      kind = ast::VariableKind::Var;
    } else if (match(lexer::TokenType::Let)) {
      kind = ast::VariableKind::Let;
    }

    // 检查是否是元组解构（接下来是 ( ）
    if (check(lexer::TokenType::LParen)) {
      // 尝试解析元组解构声明
      auto tupleDestructuringDecl =
          parseTupleDestructuringDecl(specifiers, isLate, kind);
      if (tupleDestructuringDecl) {
        auto stmt = std::make_unique<ast::TupleDestructuringStmt>(
            std::move(tupleDestructuringDecl));
        if (!attributes.empty()) {
          stmt->attributes = std::move(attributes);
        }
        return stmt;
      }
    }

    // 不是元组解构，恢复状态，尝试解析变量声明
    restoreState(tempState);
    auto varDecl = tryParseVariableDecl();
    if (varDecl) {
      auto stmt = std::make_unique<ast::VariableStmt>(std::move(varDecl));
      if (!attributes.empty()) {
        stmt->attributes = std::move(attributes);
      }
      return stmt;
    }
  }

  // 默认解析表达式语句
  return attachAttributes(parseExprStmt());
}

// 解析表达式语句
std::unique_ptr<ast::ExprStmt> Parser::parseExprStmt() {
  auto expr = parseExpression();
  expect(lexer::TokenType::Semicolon, "Expected ';' after expression");
  return std::make_unique<ast::ExprStmt>(std::move(expr));
}

// 解析返回语句
std::unique_ptr<ast::ReturnStmt> Parser::parseReturnStmt() {
  std::unique_ptr<ast::Expression> value;
  if (!check(lexer::TokenType::Semicolon)) {
    value = parseExpression();
  }
  expect(lexer::TokenType::Semicolon, "Expected ';' after return");
  return std::make_unique<ast::ReturnStmt>(std::move(value));
}

std::unique_ptr<ast::YieldStmt> Parser::parseYieldStmt() {
  std::unique_ptr<ast::Expression> value;
  if (!check(lexer::TokenType::Semicolon)) {
    value = parseExpression();
  }
  expect(lexer::TokenType::Semicolon, "Expected ';' after yield");
  return std::make_unique<ast::YieldStmt>(std::move(value));
}

// 解析复合语句
std::unique_ptr<ast::CompoundStmt> Parser::parseCompoundStmt() {
  expect(lexer::TokenType::LBrace, "Expected '{'");

  std::vector<std::unique_ptr<ast::Node>> statements;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    auto stmt = parseStatement();
    if (stmt) {
      statements.push_back(std::move(stmt));
    }
  }

  expect(lexer::TokenType::RBrace, "Expected '}'");
  return std::make_unique<ast::CompoundStmt>(std::move(statements));
}

// 解析条件语句
std::unique_ptr<ast::IfStmt> Parser::parseIfStmt() {
  expect(lexer::TokenType::LParen, "Expected '(' after if");
  auto condition = parseExpression();
  expect(lexer::TokenType::RParen, "Expected ')' after condition");

  auto thenBranch = parseStatement();

  std::unique_ptr<ast::Statement> elseBranch;
  if (match(lexer::TokenType::Else)) {
    elseBranch = parseStatement();
  }

  return std::make_unique<ast::IfStmt>(
      std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

// 解析while语句
std::unique_ptr<ast::WhileStmt> Parser::parseWhileStmt() {
  expect(lexer::TokenType::LParen, "Expected '(' after while");
  auto condition = parseExpression();
  expect(lexer::TokenType::RParen, "Expected ')' after condition");

  auto body = parseStatement();

  return std::make_unique<ast::WhileStmt>(std::move(condition),
                                          std::move(body));
}

// 解析for语句
std::unique_ptr<ast::ForStmt> Parser::parseForStmt() {
  expect(lexer::TokenType::LParen, "Expected '(' after for");

  // 检测 foreach 语法: for (var/let x : collection) 或 for (Type x :
  // collection) 通过保存状态来判断：如果 init 后面紧跟 ':' 而非 ';'，则是
  // foreach
  ParserState foreachState = saveState();
  bool isForeach = false;

  // 尝试解析迭代变量
  std::unique_ptr<ast::Node> iterVar;
  if (check(lexer::TokenType::Var) || check(lexer::TokenType::Let)) {
    advance(); // 消费 var/let
    std::string varKind =
        (previousToken->getType() == lexer::TokenType::Var) ? "var" : "let";

    if (check(lexer::TokenType::Identifier)) {
      std::string varName = currentToken->getValue();
      advance(); // 消费变量名
      if (check(lexer::TokenType::Colon)) {
        // 确认是 foreach
        isForeach = true;
        advance(); // 消费 ':'
        auto collection = parseExpression();
        expect(lexer::TokenType::RParen,
               "Expected ')' after foreach collection");
        auto body = parseStatement();

        // 创建迭代变量声明（无类型推导，无初始化）
        auto iterVarDecl = std::make_unique<ast::VariableDecl>(
            varKind, false,
            varKind == "var" ? ast::VariableKind::Var : ast::VariableKind::Let,
            nullptr, varName, nullptr, false);
        return std::make_unique<ast::ForStmt>(std::move(iterVarDecl),
                                              std::move(collection),
                                              std::move(body), true);
      }
    }
    // 不是 foreach，回退
    restoreState(foreachState);
  }

  // 普通 for 语句
  auto init = parseForInit();

  std::unique_ptr<ast::Expression> condition;
  if (!check(lexer::TokenType::Semicolon)) {
    condition = parseExpression();
  }
  expect(lexer::TokenType::Semicolon, "Expected ';' after for condition");

  std::unique_ptr<ast::Expression> update;
  if (!check(lexer::TokenType::RParen)) {
    update = parseExpression();
  }
  expect(lexer::TokenType::RParen, "Expected ')' after for update");

  auto body = parseStatement();

  return std::make_unique<ast::ForStmt>(std::move(init), std::move(condition),
                                        std::move(update), std::move(body));
}

// 解析 foreach 迭代变量
std::unique_ptr<ast::VariableDecl> Parser::parseForeachVariableDecl() {
  if (!check(lexer::TokenType::Var) && !check(lexer::TokenType::Let)) {
    error("Expected 'var' or 'let' in foreach");
    return nullptr;
  }

  std::string varKind =
      (currentToken->getType() == lexer::TokenType::Var) ? "var" : "let";
  advance();

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected identifier in foreach");
    return nullptr;
  }

  std::string varName = currentToken->getValue();
  advance();

  return std::make_unique<ast::VariableDecl>(
      varKind, false,
      varKind == "var" ? ast::VariableKind::Var : ast::VariableKind::Let,
      nullptr, varName, nullptr, false);
}

// 解析 foreach 语句: foreach (var/let name : collection) body
std::unique_ptr<ast::ForStmt> Parser::parseForeachStmt() {
  expect(lexer::TokenType::LParen, "Expected '(' after foreach");

  auto iterVarDecl = parseForeachVariableDecl();
  if (!iterVarDecl) {
    return nullptr;
  }

  std::unique_ptr<ast::VariableDecl> indexVarDecl;
  if (match(lexer::TokenType::Comma)) {
    indexVarDecl = std::move(iterVarDecl);
    iterVarDecl = parseForeachVariableDecl();
    if (!iterVarDecl) {
      return nullptr;
    }
  }

  expect(lexer::TokenType::Colon, "Expected ':' in foreach");

  auto collection = parseExpression();
  expect(lexer::TokenType::RParen, "Expected ')' after foreach collection");
  auto body = parseStatement();

  if (indexVarDecl) {
    return std::make_unique<ast::ForStmt>(
        std::move(indexVarDecl), std::move(iterVarDecl), std::move(collection),
        std::move(body), true);
  }

  return std::make_unique<ast::ForStmt>(
      std::move(iterVarDecl), std::move(collection), std::move(body), true);
}

// 解析表达式
std::unique_ptr<ast::Expression> Parser::parseExpression() {
  return parseAssignmentExpr();
}

// 解析赋值表达式
std::unique_ptr<ast::Expression> Parser::parseAssignmentExpr() {
  auto left = parseConditionalExpr();

  if (match(lexer::TokenType::Assign) || match(lexer::TokenType::AddAssign) ||
      match(lexer::TokenType::SubAssign) ||
      match(lexer::TokenType::MulAssign) ||
      match(lexer::TokenType::DivAssign) ||
      match(lexer::TokenType::ModAssign)) {
    auto opToken = previousToken->getType();
    auto right = parseAssignmentExpr();

    ast::BinaryExpr::Op op;
    switch (opToken) {
    case lexer::TokenType::Assign:
      op = ast::BinaryExpr::Op::Assign;
      break;
    case lexer::TokenType::AddAssign:
      op = ast::BinaryExpr::Op::AddAssign;
      break;
    case lexer::TokenType::SubAssign:
      op = ast::BinaryExpr::Op::SubAssign;
      break;
    case lexer::TokenType::MulAssign:
      op = ast::BinaryExpr::Op::MulAssign;
      break;
    case lexer::TokenType::DivAssign:
      op = ast::BinaryExpr::Op::DivAssign;
      break;
    case lexer::TokenType::ModAssign:
      op = ast::BinaryExpr::Op::ModAssign;
      break;
    default:
      error("Unknown assignment operator");
    }

    return std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析条件表达式
std::unique_ptr<ast::Expression> Parser::parseConditionalExpr() {
  auto condition = parseLogicalOrExpr();

  if (match(lexer::TokenType::Question)) {
    auto thenExpr = parseExpression();
    expect(lexer::TokenType::Colon, "Expected ':' in conditional expression");
    auto elseExpr = parseConditionalExpr();

    // TODO: 创建ConditionalExpr节点
    return thenExpr;
  }

  return condition;
}

// 解析逻辑或表达式
std::unique_ptr<ast::Expression> Parser::parseLogicalOrExpr() {
  auto left = parseLogicalAndExpr();

  while (match(lexer::TokenType::LogicOr)) {
    auto op = ast::BinaryExpr::Op::LogicOr;
    auto right = parseLogicalAndExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析逻辑与表达式
std::unique_ptr<ast::Expression> Parser::parseLogicalAndExpr() {
  auto left = parseInclusiveOrExpr();

  while (match(lexer::TokenType::LogicAnd)) {
    auto op = ast::BinaryExpr::Op::LogicAnd;
    auto right = parseInclusiveOrExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析包含或表达式
std::unique_ptr<ast::Expression> Parser::parseInclusiveOrExpr() {
  auto left = parseExclusiveOrExpr();

  while (match(lexer::TokenType::Or)) {
    auto op = ast::BinaryExpr::Op::Or;
    auto right = parseExclusiveOrExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析排他或表达式
std::unique_ptr<ast::Expression> Parser::parseExclusiveOrExpr() {
  auto left = parseAndExpr();

  while (match(lexer::TokenType::Xor)) {
    auto op = ast::BinaryExpr::Op::Xor;
    auto right = parseAndExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析与表达式
std::unique_ptr<ast::Expression> Parser::parseAndExpr() {
  auto left = parseEqualityExpr();

  while (match(lexer::TokenType::And)) {
    auto op = ast::BinaryExpr::Op::And;
    auto right = parseEqualityExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析相等表达式
std::unique_ptr<ast::Expression> Parser::parseEqualityExpr() {
  auto left = parseRelationalExpr();

  while (true) {
    ast::BinaryExpr::Op op;
    if (match(lexer::TokenType::Eq)) {
      op = ast::BinaryExpr::Op::Eq;
    } else if (match(lexer::TokenType::Ne)) {
      op = ast::BinaryExpr::Op::Ne;
    } else {
      break;
    }

    auto right = parseRelationalExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析关系表达式
std::unique_ptr<ast::Expression> Parser::parseRelationalExpr() {
  auto left = parseShiftExpr();

  while (true) {
    ast::BinaryExpr::Op op;
    if (match(lexer::TokenType::Lt)) {
      op = ast::BinaryExpr::Op::Lt;
    } else if (match(lexer::TokenType::Le)) {
      op = ast::BinaryExpr::Op::Le;
    } else if (match(lexer::TokenType::Gt)) {
      op = ast::BinaryExpr::Op::Gt;
    } else if (match(lexer::TokenType::Ge)) {
      op = ast::BinaryExpr::Op::Ge;
    } else {
      break;
    }

    auto right = parseShiftExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析移位表达式
std::unique_ptr<ast::Expression> Parser::parseShiftExpr() {
  auto left = parseAdditiveExpr();

  while (true) {
    ast::BinaryExpr::Op op;
    if (match(lexer::TokenType::Shl)) {
      op = ast::BinaryExpr::Op::Shl;
    } else if (match(lexer::TokenType::Shr)) {
      op = ast::BinaryExpr::Op::Shr;
    } else {
      break;
    }

    auto right = parseAdditiveExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析加法表达式
std::unique_ptr<ast::Expression> Parser::parseAdditiveExpr() {
  auto left = parseMultiplicativeExpr();

  while (true) {
    ast::BinaryExpr::Op op;
    if (match(lexer::TokenType::Plus)) {
      op = ast::BinaryExpr::Op::Add;
    } else if (match(lexer::TokenType::Minus)) {
      op = ast::BinaryExpr::Op::Sub;
    } else {
      break;
    }

    auto right = parseMultiplicativeExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析乘法表达式
std::unique_ptr<ast::Expression> Parser::parseMultiplicativeExpr() {
  auto left = parseUnaryExpr();

  while (true) {
    ast::BinaryExpr::Op op;
    if (match(lexer::TokenType::Multiply)) {
      op = ast::BinaryExpr::Op::Mul;
    } else if (match(lexer::TokenType::Divide)) {
      op = ast::BinaryExpr::Op::Div;
    } else if (match(lexer::TokenType::Modulus)) {
      op = ast::BinaryExpr::Op::Mod;
    } else {
      break;
    }

    auto right = parseUnaryExpr();
    left = std::make_unique<ast::BinaryExpr>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

// 解析一元表达式
std::unique_ptr<ast::Expression> Parser::parseUnaryExpr() {
  if (match(lexer::TokenType::Plus)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::Plus,
                                            std::move(expr));
  } else if (match(lexer::TokenType::Minus)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::Minus,
                                            std::move(expr));
  } else if (match(lexer::TokenType::Not)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::Not,
                                            std::move(expr));
  } else if (match(lexer::TokenType::BitNot)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::BitNot,
                                            std::move(expr));
  } else if (match(lexer::TokenType::AddressOf)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::AddressOf,
                                            std::move(expr));
  } else if (match(lexer::TokenType::And)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::Ref,
                                            std::move(expr));
  } else if (match(lexer::TokenType::Await)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::Await,
                                            std::move(expr));
  } else if (match(lexer::TokenType::Increment)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::PreIncrement,
                                            std::move(expr));
  } else if (match(lexer::TokenType::Decrement)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::PreDecrement,
                                            std::move(expr));
  } else if (match(lexer::TokenType::At)) {
    // 解析反射表达式: @Type 或 @typeof(expr)
    if (check(lexer::TokenType::Typeof)) {
      // @typeof(expr) 形式
      advance(); // 消费 'typeof'
      expect(lexer::TokenType::LParen, "Expected '(' after 'typeof'");
      auto expr = parseExpression();
      if (!expr) {
        error("Expected expression in typeof expression");
        return nullptr;
      }
      expect(lexer::TokenType::RParen,
             "Expected ')' after expression in typeof expression");
      return std::make_unique<ast::ReflectionExpr>(std::move(expr));
    } else {
      // @Type 形式
      auto type = parseType();
      if (!type) {
        error("Expected type after '@'");
        return nullptr;
      }
      return std::make_unique<ast::ReflectionExpr>(std::move(type));
    }
  } else if (match(lexer::TokenType::Sizeof)) {
    expect(lexer::TokenType::LParen, "Expected '(' after 'sizeof'");
    auto type = parseType();
    if (!type) {
      error("Expected type in sizeof expression");
      return nullptr;
    }
    expect(lexer::TokenType::RParen,
           "Expected ')' after type in sizeof expression");
    // 创建一个 SizeofExpr 表达式
    return std::make_unique<ast::CallExpr>(
        std::make_unique<ast::Identifier>("sizeof"),
        std::vector<std::unique_ptr<ast::Expression>>());
  } else if (match(lexer::TokenType::Typeof)) {
    expect(lexer::TokenType::LParen, "Expected '(' after 'typeof'");
    auto expr = parseExpression();
    if (!expr) {
      error("Expected expression in typeof expression");
      return nullptr;
    }
    expect(lexer::TokenType::RParen,
           "Expected ')' after expression in typeof expression");
    // 创建一个 TypeofExpr 表达式
    return std::make_unique<ast::CallExpr>(
        std::make_unique<ast::Identifier>("typeof"),
        std::vector<std::unique_ptr<ast::Expression>>());
  }

  return parsePrimaryExpr();
}

// 解析主表达式
std::unique_ptr<ast::Expression> Parser::parsePrimaryExpr() {
  std::unique_ptr<ast::Expression> expr;

  if (match(lexer::TokenType::Self)) {
    expr = std::make_unique<ast::Identifier>("self");
  } else if (match(lexer::TokenType::Base)) {
    expr = std::make_unique<ast::Identifier>("base");
  } else if (match(lexer::TokenType::Super)) {
    expr = std::make_unique<ast::Identifier>("super");
  } else if (match(lexer::TokenType::BuiltinVar)) {
    std::string name = previousToken->getValue();
    expr = std::make_unique<ast::BuiltinVarExpr>(name);
  } else if (check(lexer::TokenType::LBrace)) {
    expr = parseStructInit();
  } else if (check(lexer::TokenType::Identifier) || isTypeKeyword()) {
    ParserState state = saveState();
    auto type = parseType();
    if (type && check(lexer::TokenType::LBrace)) {
      expr = parseStructInit(std::move(type));
    } else {
      restoreState(state);
      if (match(lexer::TokenType::Identifier)) {
        std::string name = previousToken->getValue();

        // 检查是否是Lambda短语法：identifier => ...
        if (check(lexer::TokenType::FatArrow)) {
          // Lambda短语法：暂时使用空的捕获列表
          std::vector<ast::Capture> captures;

          // 创建参数
          std::vector<std::unique_ptr<ast::Parameter>> params;
          params.push_back(std::make_unique<ast::Parameter>(name, nullptr));

          advance(); // 消费 FatArrow

          // 解析表达式体
          auto bodyExpr = parseExpression();
          auto body = std::make_unique<ast::ExprStmt>(std::move(bodyExpr));

          return std::make_unique<ast::LambdaExpr>(
              std::move(params), std::move(body), std::move(captures));
        }

        // 普通标识符
        expr = std::make_unique<ast::Identifier>(name);
      } else {
        error("Expected expression");
        return nullptr;
      }
    }
  } else if (match(lexer::TokenType::IntegerLiteral)) {
    std::string value = previousToken->getValue();
    expr = std::make_unique<ast::Literal>(ast::Literal::Type::Integer, value);
  } else if (match(lexer::TokenType::FloatingLiteral)) {
    std::string value = previousToken->getValue();
    expr = std::make_unique<ast::Literal>(ast::Literal::Type::Floating, value);
  } else if (match(lexer::TokenType::StringLiteral)) {
    std::string value = previousToken->getValue();
    expr = std::make_unique<ast::Literal>(ast::Literal::Type::String, value);
  } else if (match(lexer::TokenType::True)) {
    expr = std::make_unique<ast::Literal>(ast::Literal::Type::Boolean, "true");
  } else if (match(lexer::TokenType::False)) {
    expr = std::make_unique<ast::Literal>(ast::Literal::Type::Boolean, "false");
  } else if (match(lexer::TokenType::CharacterLiteral)) {
    std::string value = previousToken->getValue();
    expr = std::make_unique<ast::Literal>(ast::Literal::Type::Character, value);
  } else if (match(lexer::TokenType::Null)) {
    expr = std::make_unique<ast::Literal>(ast::Literal::Type::Null, "null");
  } else if (match(lexer::TokenType::LParen)) {
    expr = parseTupleOrGrouping();
  } else if (match(lexer::TokenType::LBracket)) {
    // 先判断是数组初始化还是 lambda
    bool isEmptyRBracket = check(lexer::TokenType::RBracket);
    bool shouldParseLambda = false;

    if (isEmptyRBracket) {
      // 如果是空的 []，需要 lookahead 看看 ] 后面是什么
      ParserState state = saveState();
      advance(); // 消费 ]

      // 如果后面是 LParen ( 或者 FatArrow =>，那么是 Lambda
      if (check(lexer::TokenType::LParen) ||
          check(lexer::TokenType::FatArrow)) {
        shouldParseLambda = true;
      }

      restoreState(state); // 回退
    }

    if (!shouldParseLambda && (check(lexer::TokenType::LBracket) ||
                               check(lexer::TokenType::IntegerLiteral) ||
                               check(lexer::TokenType::FloatingLiteral) ||
                               check(lexer::TokenType::StringLiteral) ||
                               check(lexer::TokenType::BooleanLiteral) ||
                               check(lexer::TokenType::CharacterLiteral) ||
                               check(lexer::TokenType::Null))) {
      expr = parseArrayInit();
    } else {
      expr = parseLambdaOrArrayInit();
    }
  } else if (check(lexer::TokenType::LBrace)) {
    expr = parseStructInit();
  } else if (match(lexer::TokenType::New)) {
    expr = parseNewExpr();
  } else if (match(lexer::TokenType::Delete)) {
    expr = parseDeleteExpr();
  } else {
    error("Expected expression");
    return nullptr;
  }

  return parsePostfixExpr(std::move(expr));
}

// 解析后缀表达式
std::unique_ptr<ast::Expression>
Parser::parsePostfixExpr(std::unique_ptr<ast::Expression> expr) {
  while (true) {
    if (match(lexer::TokenType::LParen)) {
      expr = parseCallExpr(std::move(expr));
    } else if (check(lexer::TokenType::Lt)) {
      // 可能是泛型参数 <Type1, Type2, ...>，也可能是比较运算符
      // 保存状态以便回溯
      ParserState templateState = saveState();
      advance(); // 消费 '<'

      // 尝试解析泛型参数
      std::vector<std::unique_ptr<ast::Node>> templateArgs;
      bool validTemplate = true;
      if (!check(lexer::TokenType::Gt)) {
        do {
          auto argType = parseType();
          if (argType) {
            templateArgs.push_back(std::move(argType));
          } else {
            validTemplate = false;
            break;
          }
        } while (match(lexer::TokenType::Comma));
      }

      if (validTemplate && check(lexer::TokenType::Gt)) {
        advance(); // 消费 '>'
        // 将泛型参数附加到标识符，然后继续解析可能的函数调用
        if (auto *ident = dynamic_cast<ast::Identifier *>(expr.get())) {
          expr = std::make_unique<ast::Identifier>(ident->name,
                                                   std::move(templateArgs));
        }
      } else {
        // 不是有效的泛型参数列表，恢复状态
        // '<' 实际上是比较运算符，跳出循环让上层处理
        restoreState(templateState);
        break;
      }
    } else if (match(lexer::TokenType::LBracket)) {
      expr = parseSubscriptExpr(std::move(expr));
    } else if (match(lexer::TokenType::Dot)) {
      expr = parseMemberExpr(std::move(expr));
    } else if (match(lexer::TokenType::Arrow)) {
      expr = parsePointerMemberExpr(std::move(expr));
    } else if (match(lexer::TokenType::AddressOf)) {
      expr = std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::Dereference,
                                              std::move(expr));
    } else if (match(lexer::TokenType::Tilde)) {
      expr = std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::Move,
                                              std::move(expr));
    } else if (match(lexer::TokenType::Question)) {
      expr = std::make_unique<ast::UnaryExpr>(
          ast::UnaryExpr::Op::NullablePropagation, std::move(expr));
    } else if (match(lexer::TokenType::Increment)) {
      expr = std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::PostIncrement,
                                              std::move(expr));
    } else if (match(lexer::TokenType::Decrement)) {
      expr = std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::PostDecrement,
                                              std::move(expr));
    } else if (match(lexer::TokenType::Not)) {
      expr = std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::ForceUnwrap,
                                              std::move(expr));
    } else if (match(lexer::TokenType::Range)) {
      expr = parseRangeExpr(std::move(expr));
    } else if (match(lexer::TokenType::DoubleColon)) {
      expr = parseNamespaceAccessExpr(std::move(expr));
    } else {
      break;
    }
  }
  return expr;
}

// 解析函数调用
std::unique_ptr<ast::Expression>
Parser::parseCallExpr(std::unique_ptr<ast::Expression> callee) {
  std::vector<std::unique_ptr<ast::Expression>> args;

  if (!check(lexer::TokenType::RParen)) {
    do {
      args.push_back(parseExpression());
    } while (match(lexer::TokenType::Comma));
  }

  expect(lexer::TokenType::RParen, "Expected ')' after arguments");
  return std::make_unique<ast::CallExpr>(std::move(callee), std::move(args));
}

// 解析数组访问
std::unique_ptr<ast::Expression>
Parser::parseSubscriptExpr(std::unique_ptr<ast::Expression> object) {
  auto index = parseExpression();
  expect(lexer::TokenType::RBracket, "Expected ']' after index");
  return std::make_unique<ast::SubscriptExpr>(std::move(object),
                                              std::move(index));
}

// 解析成员访问
std::unique_ptr<ast::Expression>
Parser::parseMemberExpr(std::unique_ptr<ast::Expression> object) {
  if (!check(lexer::TokenType::Identifier)) {
    error("Expected identifier after '.'");
    return nullptr;
  }
  std::string member = currentToken->getValue();
  advance();
  return std::make_unique<ast::MemberExpr>(std::move(object), member, false);
}

// 解析指针成员访问
std::unique_ptr<ast::Expression>
Parser::parsePointerMemberExpr(std::unique_ptr<ast::Expression> object) {
  if (!check(lexer::TokenType::Identifier)) {
    error("Expected identifier after '->'");
    return nullptr;
  }
  std::string member = currentToken->getValue();
  advance();
  return std::make_unique<ast::MemberExpr>(std::move(object), member, true);
}

// 解析命名空间访问
std::unique_ptr<ast::Expression>
Parser::parseNamespaceAccessExpr(std::unique_ptr<ast::Expression> object) {
  if (!check(lexer::TokenType::Identifier)) {
    error("Expected identifier after '::'");
    return nullptr;
  }
  std::string member = currentToken->getValue();
  advance();
  return std::make_unique<ast::MemberExpr>(std::move(object), member, false);
}

// 解析范围表达式
std::unique_ptr<ast::Expression>
Parser::parseRangeExpr(std::unique_ptr<ast::Expression> start) {
  auto end = parsePrimaryExpr();
  return std::make_unique<ast::BinaryExpr>(
      std::move(start), ast::BinaryExpr::Op::Range, std::move(end));
}

// 解析数组初始化
std::unique_ptr<ast::Expression> Parser::parseArrayInit() {
  std::vector<std::unique_ptr<ast::Expression>> elements;

  if (!check(lexer::TokenType::RBracket)) {
    do {
      elements.push_back(parseExpression());
    } while (match(lexer::TokenType::Comma));
  }

  expect(lexer::TokenType::RBracket, "Expected ']' after array elements");
  return std::make_unique<ast::ArrayInitExpr>(std::move(elements));
}

std::unique_ptr<ast::Expression> Parser::parseLambdaOrArrayInit() {
  std::vector<ast::Capture> captures;

  if (check(lexer::TokenType::RBracket)) {
    advance();
    if (check(lexer::TokenType::LParen) || check(lexer::TokenType::FatArrow)) {
      return parseLambdaExpr();
    } else {
      return std::make_unique<ast::ArrayInitExpr>(
          std::vector<std::unique_ptr<ast::Expression>>());
    }
  }

  advance(); // Consume '[' token
  captures = parseLambdaCaptures();
  expect(lexer::TokenType::RBracket, "Expected ']' after lambda captures");

  if (check(lexer::TokenType::LParen) || check(lexer::TokenType::FatArrow)) {
    return parseLambdaExprWithCaptures(std::move(captures));
  }

  return parseArrayInit();
}

std::unique_ptr<ast::Expression>
Parser::parseLambdaExprWithCaptures(std::vector<ast::Capture> captures) {
  std::vector<std::unique_ptr<ast::Parameter>> params;

  if (match(lexer::TokenType::LParen)) {
    params = parseLambdaParams();
    expect(lexer::TokenType::RParen, "Expected ')' after lambda parameters");
  }

  std::unique_ptr<ast::Statement> body;

  if (match(lexer::TokenType::FatArrow)) {
    auto expr = parseExpression();
    body = std::make_unique<ast::ExprStmt>(std::move(expr));
  } else if (match(lexer::TokenType::LBrace)) {
    body = parseCompoundStmt();
  } else {
    error("Expected '=>' or '{' after lambda parameters");
    return nullptr;
  }

  return std::make_unique<ast::LambdaExpr>(std::move(params), std::move(body),
                                           std::move(captures));
}

// 解析元组表达式或分组表达式
std::unique_ptr<ast::Expression> Parser::parseTupleOrGrouping() {
  std::vector<std::unique_ptr<ast::Expression>> elements;

  if (!check(lexer::TokenType::RParen)) {
    do {
      elements.push_back(parseExpression());
    } while (match(lexer::TokenType::Comma));
  }

  expect(lexer::TokenType::RParen, "Expected ')' after tuple elements");

  if (elements.size() == 0) {
    error("Empty tuple is not allowed");
    return nullptr;
  }

  if (elements.size() == 1) {
    return std::move(elements[0]);
  }

  return std::make_unique<ast::TupleExpr>(std::move(elements));
}

// 解析Lambda表达式
std::unique_ptr<ast::Expression> Parser::parseLambdaExpr() {
  std::vector<std::unique_ptr<ast::Parameter>> params;

  if (check(lexer::TokenType::LParen)) {
    advance();
    params = parseLambdaParams();
    expect(lexer::TokenType::RParen, "Expected ')' after lambda parameters");
  }

  std::unique_ptr<ast::Statement> body;

  if (match(lexer::TokenType::FatArrow)) {
    auto expr = parseExpression();
    body = std::make_unique<ast::ExprStmt>(std::move(expr));
  } else if (match(lexer::TokenType::LBrace)) {
    body = parseCompoundStmt();
  } else {
    error("Expected '=>' or '{' after lambda parameters");
    return nullptr;
  }

  return std::make_unique<ast::LambdaExpr>(std::move(params), std::move(body));
}

std::vector<ast::Capture> Parser::parseLambdaCaptures() {
  std::vector<ast::Capture> captures;

  // 检查是否是空的捕获列表 []
  if (check(lexer::TokenType::RBracket)) {
    return captures;
  }

  do {
    bool byRef = false;
    bool isMove = false;

    if (match(lexer::TokenType::And)) {
      byRef = true;
    }

    if (!check(lexer::TokenType::Identifier) &&
        !check(lexer::TokenType::Assign)) {
      error("Expected identifier or '=' in lambda capture");
      return captures;
    }

    if (match(lexer::TokenType::Assign)) {
      isMove = true;
    }

    if (check(lexer::TokenType::Identifier)) {
      std::string name = currentToken->getValue();
      advance();
      captures.push_back(ast::Capture(name, byRef, isMove));
    }
  } while (match(lexer::TokenType::Comma));

  return captures;
}

// 解析Lambda参数
std::vector<std::unique_ptr<ast::Parameter>> Parser::parseLambdaParams() {
  std::vector<std::unique_ptr<ast::Parameter>> params;

  if (!check(lexer::TokenType::RParen)) {
    do {
      std::unique_ptr<ast::Type> type;
      std::string name;

      type = parseType();

      if (!check(lexer::TokenType::Identifier)) {
        error("Expected parameter name after type");
        return params;
      }

      name = currentToken->getValue();
      advance();

      std::unique_ptr<ast::Expression> defaultValue;
      if (match(lexer::TokenType::Assign)) {
        defaultValue = parseExpression();
      }

      params.push_back(std::make_unique<ast::Parameter>(
          name, std::move(type), std::move(defaultValue)));
    } while (match(lexer::TokenType::Comma));
  }

  return params;
}

// 解析结构体初始化
std::unique_ptr<ast::Expression>
Parser::parseStructInit(std::unique_ptr<ast::Type> type) {
  std::vector<std::pair<std::string, std::unique_ptr<ast::Expression>>> fields;

  // 先消费 {
  if (!match(lexer::TokenType::LBrace)) {
    error("Expected '{' at start of struct initializer");
    return nullptr;
  }

  if (!check(lexer::TokenType::RBrace)) {
    do {
      // 尝试解析具名字段初始化：identifier : value 或 identifier = value
      // 如果不是具名形式，则当作位置初始化（直接是表达式）
      ParserState fieldState = saveState();
      std::string name = "";
      bool isNamedField = false;

      if (check(lexer::TokenType::Identifier)) {
        name = currentToken->getValue();
        advance();
        if (match(lexer::TokenType::Colon) || match(lexer::TokenType::Assign)) {
          isNamedField = true;
        } else {
          // 不是具名字段，回滚
          restoreState(fieldState);
        }
      }

      auto value = parseExpression();
      fields.push_back({name, std::move(value)});
    } while (match(lexer::TokenType::Comma));
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after struct fields");
  if (type) {
    return std::make_unique<ast::StructInitExpr>(std::move(type),
                                                 std::move(fields));
  } else {
    return std::make_unique<ast::StructInitExpr>(std::move(fields));
  }
}

// 解析类型
// 解析基本类型（不包括类型后缀）
std::unique_ptr<ast::Type> Parser::parseBasicType() {
  if (!check(lexer::TokenType::Identifier) && !isTypeKeyword()) {
    return nullptr;
  }

  // 处理函数指针类型: func(param_types) -> ret_type
  if (check(lexer::TokenType::Func)) {
    advance(); // 消费 'func'

    expect(lexer::TokenType::LParen, "Expected '(' after 'func'");

    std::vector<std::unique_ptr<ast::Type>> paramTypes;
    if (!check(lexer::TokenType::RParen)) {
      do {
        auto paramType = parseType();
        if (!paramType) {
          error("Expected parameter type in function pointer type");
          return nullptr;
        }
        paramTypes.push_back(std::move(paramType));
      } while (match(lexer::TokenType::Comma));
    }

    expect(lexer::TokenType::RParen,
           "Expected ')' after function parameter types");

    std::unique_ptr<ast::Type> returnType;
    if (match(lexer::TokenType::Arrow)) {
      returnType = parseType();
      if (!returnType) {
        error("Expected return type after '->' in function pointer type");
        return nullptr;
      }
    } else {
      returnType =
          std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Void);
    }

    return std::make_unique<ast::FunctionType>(std::move(paramTypes),
                                               std::move(returnType));
  }

  std::string name = currentToken->getValue();
  advance();

  std::unique_ptr<ast::Type> type;

  if (name == "void") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Void);
  } else if (name == "bool") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Bool);
  } else if (name == "byte") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Byte);
  } else if (name == "sbyte") {
    type =
        std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::SByte);
  } else if (name == "short") {
    type =
        std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Short);
  } else if (name == "ushort") {
    type =
        std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::UShort);
  } else if (name == "int" || name == "int32") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Int);
  } else if (name == "uint" || name == "uint32") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::UInt);
  } else if (name == "long" || name == "int64") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Long);
  } else if (name == "ulong" || name == "uint64") {
    type =
        std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::ULong);
  } else if (name == "float") {
    type =
        std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Float);
  } else if (name == "double") {
    type =
        std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Double);
  } else if (name == "fp16") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Fp16);
  } else if (name == "bf16") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Bf16);
  } else if (name == "char") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Char);
  } else if (name == "literalview") {
    type = std::make_unique<ast::NamedType>("literalview");
  } else {
    type = std::make_unique<ast::NamedType>(name);
  }

  // 检查是否有泛型参数 <Type1, Type2, ...>
  if (check(lexer::TokenType::Lt)) {
    ParserState genericState = saveState();
    advance(); // 消费 '<'

    std::vector<std::unique_ptr<ast::Node>> arguments;
    bool validGeneric = true;
    if (!check(lexer::TokenType::Gt)) {
      do {
        auto argType = parseType();
        if (argType) {
          arguments.push_back(std::move(argType));
        } else {
          validGeneric = false;
          break;
        }
      } while (match(lexer::TokenType::Comma));
    }

    if (validGeneric && check(lexer::TokenType::Gt)) {
      advance(); // 消费 '>'
      // 创建泛型类型实例
      type = std::make_unique<ast::GenericType>(name, std::move(arguments));
    } else {
      // 不是有效的泛型参数列表，恢复状态
      restoreState(genericState);
    }
  }

  // 检查是否是只读类型
  if (match(lexer::TokenType::Not)) {
    type = std::make_unique<ast::ReadonlyType>(std::move(type));
  }

  return type;
}

std::unique_ptr<ast::Type> Parser::parseType() {
  // 检查是否是 (Type1, Type2, ...) 元组类型
  if (check(lexer::TokenType::LParen)) {
    advance(); // 消费 LParen

    std::vector<std::unique_ptr<ast::Type>> elementTypes;
    if (!check(lexer::TokenType::RParen)) {
      do {
        // 使用 parseBasicType 而不是 parseType，避免消耗逗号
        elementTypes.push_back(parseBasicType());
      } while (match(lexer::TokenType::Comma));
    }

    expect(lexer::TokenType::RParen, "Expected ')' after tuple type elements");

    if (elementTypes.size() < 2) {
      error("Tuple type must have at least 2 elements");
      return nullptr;
    }

    // 检查是否是只读类型
    std::unique_ptr<ast::Type> tupleType =
        std::make_unique<ast::TupleType>(std::move(elementTypes));
    if (match(lexer::TokenType::Not)) {
      tupleType = std::make_unique<ast::ReadonlyType>(std::move(tupleType));
    }

    return parseTypeSuffix(std::move(tupleType));
  }

  // 检查是否是 func(...) -> ... 函数类型
  if (check(lexer::TokenType::Func)) {
    advance();
    expect(lexer::TokenType::LParen, "Expected '(' after func");

    std::vector<std::unique_ptr<ast::Type>> parameterTypes;
    if (!check(lexer::TokenType::RParen)) {
      do {
        parameterTypes.push_back(parseType());
      } while (match(lexer::TokenType::Comma));
    }
    expect(lexer::TokenType::RParen, "Expected ')' after function parameters");

    std::unique_ptr<ast::Type> returnType;
    if (match(lexer::TokenType::Arrow)) {
      returnType = parseType();
    } else {
      returnType =
          std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Void);
    }
    return std::make_unique<ast::FunctionType>(std::move(parameterTypes),
                                               std::move(returnType));
  }

  auto type = parseBasicType();
  if (!type) {
    return nullptr;
  }

  type = parseTypeSuffix(std::move(type));

  return type;
}

// 解析类型后缀（指针、引用、数组、切片、泛型、可空、只读）
std::unique_ptr<ast::Type>
Parser::parseTypeSuffix(std::unique_ptr<ast::Type> type) {
  while (true) {
    if (match(lexer::TokenType::AddressOf)) {
      type = std::make_unique<ast::PointerType>(std::move(type), false);
    } else if (match(lexer::TokenType::And)) {
      type = std::make_unique<ast::ReferenceType>(std::move(type));
    } else if (match(lexer::TokenType::Question)) {
      type = std::make_unique<ast::NullableType>(std::move(type));
    } else if (match(lexer::TokenType::Not)) {
      // ! 是左绑定的不可变修饰符，修饰左边的类型
      type = std::make_unique<ast::ReadonlyType>(std::move(type));
    } else if (match(lexer::TokenType::LBracket)) {
      // 解析 [ ... ] 中的内容，可能是多维的
      if (match(lexer::TokenType::RBracket)) {
        // 单个 [] → 一维切片
        type = std::make_unique<ast::SliceType>(std::move(type));
      } else if (match(lexer::TokenType::Dollar)) {
        // [$] → 自动推导大小的栈数组
        expect(lexer::TokenType::RBracket,
               "Expected ']' after $ in array type");
        // 这里 size 设为 nullptr，语义分析阶段从初始化表达式推导
        type = std::make_unique<ast::ArrayType>(std::move(type), nullptr);
      } else if (match(lexer::TokenType::Comma)) {
        // 以逗号开头 → 多维切片，如 [, ] 或 [,,]
        int rank = 1; // 第一个隐含的维度
        while (match(lexer::TokenType::Comma)) {
          rank++;
        }
        expect(lexer::TokenType::RBracket,
               "Expected ']' after comma-separated dimensions in slice type");
        type = std::make_unique<ast::RectangularSliceType>(std::move(type),
                                                           rank + 1);
      } else {
        // 可能有一个或多个大小，可能带逗号
        std::vector<std::unique_ptr<ast::Expression>> sizes;

        // 解析第一个大小
        sizes.push_back(parseExpression());

        // 检查是否有逗号，表示多维数组
        bool isRectangular = false;
        while (match(lexer::TokenType::Comma)) {
          isRectangular = true;
          sizes.push_back(parseExpression());
        }

        expect(lexer::TokenType::RBracket, "Expected ']' after array sizes");

        if (isRectangular) {
          // 矩形数组
          type = std::make_unique<ast::RectangularArrayType>(std::move(type),
                                                             std::move(sizes));
        } else {
          // 一维数组
          type = std::make_unique<ast::ArrayType>(std::move(type),
                                                  std::move(sizes[0]));
        }
      }
    } else if (check(lexer::TokenType::Lt)) {
      // 尝试解析模板参数列表，但需要先检查是否是有效的模板参数
      ParserState state = saveState();
      advance(); // 消费 '<'

      // 尝试解析模板实参
      auto args = parseTemplateArguments();

      if (check(lexer::TokenType::Gt)) {
        advance(); // 消费 '>'
        type = std::make_unique<ast::GenericType>(type->toString(),
                                                  std::move(args));
      } else {
        // 不是有效的模板参数列表，恢复状态
        restoreState(state);
        break;
      }
    } else {
      break;
    }
  }
  return type;
}

// 解析参数列表
std::vector<std::unique_ptr<ast::Node>> Parser::parseParameterList() {
  std::vector<std::unique_ptr<ast::Node>> params;

  if (!check(lexer::TokenType::RParen)) {
    do {
      auto param = parseParameter();
      if (!param) {
        return {};
      }
      params.push_back(std::move(param));
    } while (match(lexer::TokenType::Comma));
  }

  return params;
}

// 解析参数
std::unique_ptr<ast::Node> Parser::parseParameter() {
  bool isVariadic = false;
  bool isSelf = false;
  bool isSelfImmutable = false;

  // 检查是否是变参（... 在参数开头）
  if (match(lexer::TokenType::Ellipsis)) {
    isVariadic = true;
    return std::make_unique<ast::Parameter>("", nullptr, nullptr, true);
  }

  // 检查是否是 self 或 self! 参数
  if (check(lexer::TokenType::Self)) {
    advance();
    isSelf = true;
    if (match(lexer::TokenType::Not)) {
      isSelfImmutable = true;
    }
    // self 参数可以有名字，也可以没有（通常就叫 self）
    std::string name = "self";
    if (check(lexer::TokenType::Identifier)) {
      name = currentToken->getValue();
      advance();
    }
    return std::make_unique<ast::Parameter>(name, std::unique_ptr<ast::Type>(),
                                            nullptr, false, isSelf,
                                            isSelfImmutable);
  }

  if (!check(lexer::TokenType::Identifier) && !isTypeKeyword() &&
      !check(lexer::TokenType::LParen)) {
    error("Expected parameter type");
    return nullptr;
  }

  auto type = parseType();

  // 检查是否是变参（... 在类型之后）
  if (match(lexer::TokenType::Ellipsis)) {
    isVariadic = true;
  }

  std::string name;
  if (check(lexer::TokenType::Identifier)) {
    name = currentToken->getValue();
    advance();
  }

  std::unique_ptr<ast::Expression> defaultValue;
  if (match(lexer::TokenType::Assign)) {
    defaultValue = parseExpression();
  }

  return std::make_unique<ast::Parameter>(name, std::move(type),
                                          std::move(defaultValue), isVariadic);
}

// 解析参数包
std::unique_ptr<ast::Node> Parser::parseParameterPack() {
  if (!check(lexer::TokenType::Identifier)) {
    error("Expected parameter pack name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  return std::make_unique<ast::TemplateParameter>(name, nullptr, true);
}

// 解析模板参数
std::vector<std::unique_ptr<ast::TemplateParameter>>
Parser::parseTemplateParameters() {
  std::vector<std::unique_ptr<ast::TemplateParameter>> params;

  if (!check(lexer::TokenType::Gt)) {
    do {
      if (!check(lexer::TokenType::Identifier)) {
        error("Expected template parameter name");
        return params;
      }

      std::string name = currentToken->getValue();
      advance();

      std::unique_ptr<ast::Node> constraint;
      if (match(lexer::TokenType::Colon)) {
        constraint = parseType();
      }

      bool isVariadic = match(lexer::TokenType::Ellipsis);

      params.push_back(std::make_unique<ast::TemplateParameter>(
          name, std::move(constraint), isVariadic));
    } while (match(lexer::TokenType::Comma));
  }

  return params;
}

// 解析模板实参
std::vector<std::unique_ptr<ast::Node>> Parser::parseTemplateArguments() {
  std::vector<std::unique_ptr<ast::Node>> args;

  if (!check(lexer::TokenType::Gt)) {
    do {
      if (check(lexer::TokenType::Identifier)) {
        args.push_back(parseType());
      } else {
        args.push_back(parseExpression());
      }
    } while (match(lexer::TokenType::Comma));
  }

  return args;
}

// 解析where子句
std::unique_ptr<ast::Node> Parser::parseWhereClause() {
  // where 子句可以包含多个约束，用逗号分隔
  // 例如: where Integral<T>, Addable<T>
  std::vector<std::unique_ptr<ast::Node>> constraints;

  do {
    if (!check(lexer::TokenType::Identifier)) {
      error("Expected constraint in where clause");
      return nullptr;
    }

    auto constraint = parseType();
    if (constraint) {
      constraints.push_back(std::move(constraint));
    }
  } while (match(lexer::TokenType::Comma));

  // 如果只有一个约束，直接返回
  if (constraints.size() == 1) {
    return std::move(constraints[0]);
  }

  // 多个约束时，返回第一个（简化处理）
  // TODO: 实现完整的约束列表
  if (!constraints.empty()) {
    return std::move(constraints[0]);
  }

  return nullptr;
}

// 解析requires子句
std::unique_ptr<ast::Node> Parser::parseRequiresClause() {
  return parseRequiresExpression();
}

// 解析requires表达式
std::unique_ptr<ast::RequiresExpression> Parser::parseRequiresExpression() {
  expect(lexer::TokenType::LParen, "Expected '(' after requires");

  std::vector<std::unique_ptr<ast::Node>> params;
  std::vector<std::unique_ptr<ast::Requirement>> requirements;

  if (!check(lexer::TokenType::RParen)) {
    requirements = parseRequirementList();
  }

  expect(lexer::TokenType::RParen, "Expected ')' after requires expression");
  return std::make_unique<ast::RequiresExpression>(std::move(params),
                                                   std::move(requirements));
}

// 解析需求
std::unique_ptr<ast::Requirement> Parser::parseRequirement() {
  if (check(lexer::TokenType::Identifier)) {
    auto type = parseType();

    if (match(lexer::TokenType::LParen)) {
      std::vector<std::unique_ptr<ast::Expression>> args;

      if (!check(lexer::TokenType::RParen)) {
        do {
          args.push_back(parseExpression());
        } while (match(lexer::TokenType::Comma));
      }

      expect(lexer::TokenType::RParen,
             "Expected ')' after requirement arguments");

      if (args.size() == 1) {
        return std::make_unique<ast::Requirement>(
            ast::Requirement::RequirementType::Compound, std::move(args[0]),
            std::move(type));
      }

      return std::make_unique<ast::Requirement>(
          ast::Requirement::RequirementType::Typename, std::move(type));
    }

    return std::make_unique<ast::Requirement>(
        ast::Requirement::RequirementType::Typename, std::move(type));
  }

  error("Expected requirement");
  return nullptr;
}

// 解析需求列表
std::vector<std::unique_ptr<ast::Requirement>> Parser::parseRequirementList() {
  std::vector<std::unique_ptr<ast::Requirement>> requirements;

  do {
    requirements.push_back(parseRequirement());
  } while (match(lexer::TokenType::Comma));

  return requirements;
}

// 检查当前token是否为类型开始
bool Parser::isTypeStart() const {
  if (!currentToken) {
    return false;
  }

  return isTypeKeyword() ||
         currentToken->getType() == lexer::TokenType::Identifier ||
         currentToken->getType() == lexer::TokenType::BuiltinVar ||
         currentToken->getType() == lexer::TokenType::LParen;
}

// 检查当前token是否为类型关键字
bool Parser::isTypeKeyword() const {
  if (!currentToken) {
    return false;
  }

  auto type = currentToken->getType();
  return type == lexer::TokenType::Void || type == lexer::TokenType::Bool ||
         type == lexer::TokenType::Byte || type == lexer::TokenType::SByte ||
         type == lexer::TokenType::Short || type == lexer::TokenType::UShort ||
         type == lexer::TokenType::Int || type == lexer::TokenType::UInt ||
         type == lexer::TokenType::Long || type == lexer::TokenType::ULong ||
         type == lexer::TokenType::Float || type == lexer::TokenType::Double ||
         type == lexer::TokenType::Fp16 || type == lexer::TokenType::Bf16 ||
         type == lexer::TokenType::Char ||
         type == lexer::TokenType::LiteralView ||
         type == lexer::TokenType::Func;
}

// 其他TODO函数
std::unique_ptr<ast::Node> Parser::parseForInit() {
  if (check(lexer::TokenType::Semicolon)) {
    advance();
    return nullptr;
  }

  if (check(lexer::TokenType::Var) || check(lexer::TokenType::Let) ||
      check(lexer::TokenType::Late) || isTypeStart()) {
    auto decl = parseVariableDeclForFor();
    expect(lexer::TokenType::Semicolon, "Expected ';' after for init");
    return decl;
  }

  auto expr = parseExpression();
  expect(lexer::TokenType::Semicolon, "Expected ';' after for init");
  return expr;
}

std::unique_ptr<ast::DoWhileStmt> Parser::parseDoWhileStmt() {
  auto body = parseStatement();
  expect(lexer::TokenType::While, "Expected 'while' after do-while body");
  expect(lexer::TokenType::LParen, "Expected '(' after while");
  auto condition = parseExpression();
  expect(lexer::TokenType::RParen, "Expected ')' after condition");
  expect(lexer::TokenType::Semicolon, "Expected ';' after do-while statement");

  return std::make_unique<ast::DoWhileStmt>(std::move(body),
                                            std::move(condition));
}

std::unique_ptr<ast::MatchStmt> Parser::parseMatchStmt() {
  expect(lexer::TokenType::LParen, "Expected '(' after match");
  auto expr = parseExpression();
  expect(lexer::TokenType::RParen, "Expected ')' after match expression");
  expect(lexer::TokenType::LBrace, "Expected '{' after match expression");

  std::vector<std::unique_ptr<ast::MatchArm>> arms;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    arms.push_back(parseMatchArm());
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after match arms");

  return std::make_unique<ast::MatchStmt>(std::move(expr), std::move(arms));
}

std::unique_ptr<ast::MatchArm> Parser::parseMatchArm() {
  auto pattern = parsePattern();
  if (!pattern) {
    error("Failed to parse pattern in match arm");
    return nullptr;
  }

  std::unique_ptr<ast::Expression> guard;
  if (match(lexer::TokenType::If)) {
    guard = parseExpression();
  }

  expect(lexer::TokenType::FatArrow, "Expected '=>' after match pattern");

  std::unique_ptr<ast::Statement> body;
  if (check(lexer::TokenType::LBrace)) {
    body = parseCompoundStmt();
  } else {
    body = parseExprStmt();
  }

  return std::make_unique<ast::MatchArm>(std::move(pattern), std::move(guard),
                                         std::move(body));
}

std::unique_ptr<ast::Pattern> Parser::parsePattern() {
  // 检查 _ 作为 default pattern
  if (check(lexer::TokenType::Identifier)) {
    std::string val = currentToken->getValue();
    if (val == "_") {
      advance();
      auto pattern = std::make_unique<ast::Pattern>();
      pattern->isDefault = true;
      return pattern;
    }
    advance();
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::IntegerLiteral)) {
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::StringLiteral)) {
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::BooleanLiteral)) {
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::CharacterLiteral)) {
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::Null)) {
    return std::make_unique<ast::Pattern>();
  }

  error("Expected pattern");
  return nullptr;
}

std::unique_ptr<ast::Statement> Parser::parseThrowStmt() {
  std::unique_ptr<ast::Expression> expr;
  if (!check(lexer::TokenType::Semicolon)) {
    expr = parseExpression();
  }
  expect(lexer::TokenType::Semicolon, "Expected ';' after throw");
  return std::make_unique<ast::ThrowStmt>(std::move(expr));
}

std::unique_ptr<ast::Statement> Parser::parseTryStmt() {
  auto tryBlock = parseStatement();

  std::vector<std::unique_ptr<ast::CatchStmt>> catchStmts;
  while (match(lexer::TokenType::Catch)) {
    catchStmts.push_back(parseCatchStmt());
  }

  return std::make_unique<ast::TryStmt>(std::move(tryBlock),
                                        std::move(catchStmts));
}

std::unique_ptr<ast::CatchStmt> Parser::parseCatchStmt() {
  expect(lexer::TokenType::LParen, "Expected '(' after catch");

  std::unique_ptr<ast::Parameter> param;

  // 检查是否是 catch(...)
  if (check(lexer::TokenType::Ellipsis)) {
    advance();
    param = std::make_unique<ast::Parameter>("...", nullptr, nullptr);
  } else {
    // 正常的 catch(Type var)
    auto paramNode = parseParameter();
    if (paramNode->getType() == ast::NodeType::Parameter) {
      param = std::unique_ptr<ast::Parameter>(
          static_cast<ast::Parameter *>(paramNode.release()));
    } else {
      error("Expected parameter in catch");
      return nullptr;
    }
  }

  expect(lexer::TokenType::RParen, "Expected ')' after catch parameter");

  auto body = parseStatement();

  return std::make_unique<ast::CatchStmt>(std::move(param), std::move(body));
}

std::unique_ptr<ast::Statement> Parser::parseDeferStmt() {
  auto expr = parseExpression();
  expect(lexer::TokenType::Semicolon, "Expected ';' after defer expression");
  return std::make_unique<ast::DeferStmt>(std::move(expr));
}

std::unique_ptr<ast::ComptimeStmt> Parser::parseComptimeStmt() {
  // 解析 comptime 后面的语句
  auto stmt = parseStatement();
  return std::make_unique<ast::ComptimeStmt>(std::move(stmt));
}

std::unique_ptr<ast::FoldExpr> Parser::parseFoldExpr() { return nullptr; }

std::unique_ptr<ast::ExpansionExpr> Parser::parseExpansionExpr() {
  return nullptr;
}

// 保存解析器状态
Parser::ParserState Parser::saveState() {
  return ParserState{currentToken, previousToken, lexer.saveState()};
}

// 解析外部声明块
std::unique_ptr<ast::Declaration> Parser::parseExternDecl() {
  expect(lexer::TokenType::StringLiteral,
         "Expected string literal after 'extern'");
  std::string abi = previousToken->getValue();

  // 去掉字符串字面量的引号
  if (abi.size() >= 2 && abi.front() == '"' && abi.back() == '"') {
    abi = abi.substr(1, abi.size() - 2);
  }

  // 检查是 extern 块还是单个函数
  if (match(lexer::TokenType::LBrace)) {
    // extern 块
    std::vector<std::unique_ptr<ast::Declaration>> declarations;
    while (!check(lexer::TokenType::RBrace) &&
           !check(lexer::TokenType::EndOfFile)) {
      if (auto decl = parseDeclaration()) {
        declarations.push_back(std::move(decl));
      } else {
        advance();
      }
    }

    expect(lexer::TokenType::RBrace, "Expected '}' after extern block");

    return std::make_unique<ast::ExternDecl>(std::move(abi),
                                             std::move(declarations));
  } else {
    // 单个 extern 函数
    if (auto funcDecl = parseFunctionDecl()) {
      // 创建一个只包含单个函数的 extern 声明
      std::vector<std::unique_ptr<ast::Declaration>> declarations;
      declarations.push_back(std::move(funcDecl));
      return std::make_unique<ast::ExternDecl>(std::move(abi),
                                               std::move(declarations));
    } else {
      error("Expected function declaration after extern");
      return nullptr;
    }
  }
}

// 解析new表达式
std::unique_ptr<ast::Expression> Parser::parseNewExpr() {
  auto type = parseType();
  std::vector<std::unique_ptr<ast::Expression>> args;

  if (match(lexer::TokenType::LParen)) {
    if (!check(lexer::TokenType::RParen)) {
      do {
        args.push_back(parseExpression());
      } while (match(lexer::TokenType::Comma));
    }
    expect(lexer::TokenType::RParen, "Expected ')' after new arguments");
  }

  return std::make_unique<ast::NewExpr>(std::move(type), std::move(args));
}

// 解析delete表达式
std::unique_ptr<ast::Expression> Parser::parseDeleteExpr() {
  bool isArray = match(lexer::TokenType::LBracket);
  if (isArray) {
    expect(lexer::TokenType::RBracket, "Expected ']' after delete[]");
  }
  auto expr = parseExpression();
  return std::make_unique<ast::DeleteExpr>(std::move(expr), isArray);
}

// 恢复解析器状态
void Parser::restoreState(const ParserState &state) {
  currentToken = state.currentToken;
  previousToken = state.previousToken;
  lexer.restoreState(state.lexerState);
}

} // namespace parser
} // namespace c_hat
