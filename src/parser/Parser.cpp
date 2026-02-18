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
  // 模块声明必须在最前面
  if (match(lexer::TokenType::Module)) {
    return parseModuleDecl();
  }

  // 外部声明块
  if (match(lexer::TokenType::Extern)) {
    return parseExternDecl();
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
  ParserState tryState = saveState();
  // -1. 尝试解析 Getter 声明
  auto getterDecl = parseGetterDecl();
  if (getterDecl) {
    return getterDecl;
  }
  restoreState(tryState);

  tryState = saveState();
  // -0.5. 尝试解析 Setter 声明
  auto setterDecl = parseSetterDecl();
  if (setterDecl) {
    return setterDecl;
  }
  restoreState(tryState);

  tryState = saveState();
  // 0. 尝试解析扩展声明
  auto extensionDecl = parseExtensionDecl();
  if (extensionDecl) {
    return extensionDecl;
  }
  restoreState(tryState);

  // 1. 尝试解析函数声明
  tryState = saveState();
  auto funcDecl = parseFunctionDecl();
  if (funcDecl) {
    return funcDecl;
  }
  restoreState(tryState);

  // 3. 尝试解析类声明
  tryState = saveState();
  auto classDecl = parseClassDecl();
  if (classDecl) {
    return classDecl;
  }
  restoreState(tryState);

  // 4. 尝试解析结构体声明
  tryState = saveState();
  auto structDecl = parseStructDecl();
  if (structDecl) {
    return structDecl;
  }
  restoreState(tryState);

  // 5. 尝试解析枚举声明
  tryState = saveState();
  auto enumDecl = parseEnumDecl();
  if (enumDecl) {
    return enumDecl;
  }
  restoreState(tryState);

  // 6. 尝试解析类型别名声明
  tryState = saveState();
  auto usingDecl = parseTypeAliasDecl();
  if (usingDecl) {
    return usingDecl;
  }
  restoreState(tryState);

  // 7. 尝试解析 const 声明
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

    return std::make_unique<ast::VariableDecl>(
        specifiers, false, ast::VariableKind::Let, std::move(type), name,
        std::move(initializer), true);
  }

  // 8. 尝试解析变量声明
  if (check(lexer::TokenType::Var) || check(lexer::TokenType::Let) ||
      check(lexer::TokenType::Late) || isTypeStart() ||
      check(lexer::TokenType::Public) || check(lexer::TokenType::Private) ||
      check(lexer::TokenType::Protected) || check(lexer::TokenType::Internal)) {
    auto varDecl = tryParseVariableDecl();
    if (varDecl) {
      return varDecl;
    }
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

// 解析变量声明
std::unique_ptr<ast::VariableDecl> Parser::parseVariableDecl() {
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
    error("Expected identifier in variable declaration");
    return nullptr;
  }
  std::string name = currentToken->getValue();
  advance();

  std::unique_ptr<ast::Expression> initializer;
  if (match(lexer::TokenType::Assign)) {
    initializer = parseExpression();
  }

  expect(lexer::TokenType::Semicolon,
         "Expected ';' after variable declaration");

  return std::make_unique<ast::VariableDecl>(
      specifiers, isLate, kind, std::move(type), name, std::move(initializer));
}

// 尝试解析变量声明（失败时返回 nullptr，不抛异常）
std::unique_ptr<ast::VariableDecl> Parser::tryParseVariableDecl() {
  ParserState state = saveState();
  try {
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
      restoreState(state);
      return nullptr;
    }
    std::string name = currentToken->getValue();
    advance();

    std::unique_ptr<ast::Expression> initializer;
    if (match(lexer::TokenType::Assign)) {
      initializer = parseExpression();
    }

    if (!check(lexer::TokenType::Semicolon)) {
      restoreState(state);
      return nullptr;
    }
    advance();

    return std::make_unique<ast::VariableDecl>(specifiers, isLate, kind,
                                               std::move(type), name,
                                               std::move(initializer));
  } catch (...) {
    restoreState(state);
    return nullptr;
  }
}

// 解析函数声明
std::unique_ptr<ast::FunctionDecl> Parser::parseFunctionDecl() {
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

  // 检查并消费 Func 关键字
  if (!match(lexer::TokenType::Func)) {
    return nullptr; // 不是函数声明，返回 nullptr
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected function name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

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

  std::unique_ptr<ast::Node> returnType;
  if (match(lexer::TokenType::Arrow)) {
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

  if (match(lexer::TokenType::FatArrow)) {
    arrowExpr = parseExpression();
    expect(lexer::TokenType::Semicolon, "Expected ';' after arrow expression");
  } else if (check(lexer::TokenType::LBrace)) {
    body = parseCompoundStmt();
  } else {
    expect(lexer::TokenType::Semicolon,
           "Expected ';' after function declaration");
  }

  return std::make_unique<ast::FunctionDecl>(
      specifiers, name, std::move(templateParams), std::move(params),
      std::move(returnType), std::move(whereClause), std::move(requiresClause),
      std::move(body), std::move(arrowExpr));
}

// 解析类声明
std::unique_ptr<ast::ClassDecl> Parser::parseClassDecl() {
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

  std::string baseClass;
  if (match(lexer::TokenType::Colon)) {
    if (!check(lexer::TokenType::Identifier)) {
      error("Expected base class name");
      return nullptr;
    }
    baseClass = currentToken->getValue();
    advance();
  }

  std::vector<std::string> interfaces;
  if (match(lexer::TokenType::Colon)) {
    do {
      if (!check(lexer::TokenType::Identifier)) {
        error("Expected interface name");
        return nullptr;
      }
      interfaces.push_back(currentToken->getValue());
      advance();
    } while (match(lexer::TokenType::Comma));
  }

  expect(lexer::TokenType::LBrace, "Expected '{' after class declaration");

  std::vector<std::unique_ptr<ast::Node>> members;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    members.push_back(parseDeclaration());
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after class body");

  return std::make_unique<ast::ClassDecl>(
      specifiers, name, baseClass, std::move(interfaces), std::move(members));
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
  std::string specifiers = "";

  // 解析访问修饰符和 static
  while (true) {
    if (match(lexer::TokenType::Public)) {
      specifiers += "public ";
    } else if (match(lexer::TokenType::Private)) {
      specifiers += "private ";
    } else if (match(lexer::TokenType::Protected)) {
      specifiers += "protected ";
    } else if (match(lexer::TokenType::Internal)) {
      specifiers += "internal ";
    } else if (match(lexer::TokenType::Static)) {
      specifiers += "static ";
    } else {
      break;
    }
  }

  // 检查并消费 Get 关键字
  if (!match(lexer::TokenType::Get)) {
    return nullptr;
  }

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected getter name");
    return nullptr;
  }
  std::string name = currentToken->getValue();
  advance();

  // 解析返回类型
  std::unique_ptr<ast::Node> returnType;
  if (match(lexer::TokenType::Arrow)) {
    returnType = parseType();
    if (!returnType) {
      error("Expected return type after '->'");
      return nullptr;
    }
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
    expect(lexer::TokenType::Semicolon,
           "Expected ';' after getter declaration");
  }

  return std::make_unique<ast::GetterDecl>(
      specifiers, name, std::move(returnType), std::move(body),
      std::move(arrowExpr));
}

// 解析 Setter 声明
std::unique_ptr<ast::SetterDecl> Parser::parseSetterDecl() {
  std::string specifiers = "";

  // 解析访问修饰符和 static
  while (true) {
    if (match(lexer::TokenType::Public)) {
      specifiers += "public ";
    } else if (match(lexer::TokenType::Private)) {
      specifiers += "private ";
    } else if (match(lexer::TokenType::Protected)) {
      specifiers += "protected ";
    } else if (match(lexer::TokenType::Internal)) {
      specifiers += "internal ";
    } else if (match(lexer::TokenType::Static)) {
      specifiers += "static ";
    } else {
      break;
    }
  }

  // 检查并消费 Set 关键字
  if (!match(lexer::TokenType::Set)) {
    return nullptr;
  }

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

  expect(lexer::TokenType::Semicolon,
         "Expected ';' after type alias declaration");

  return std::make_unique<ast::TypeAliasDecl>(specifiers, name,
                                              std::move(type));
}

// 解析语句
std::unique_ptr<ast::Statement> Parser::parseStatement() {
  if (check(lexer::TokenType::LBrace)) {
    return parseCompoundStmt();
  } else if (match(lexer::TokenType::If)) {
    return parseIfStmt();
  } else if (match(lexer::TokenType::While)) {
    return parseWhileStmt();
  } else if (match(lexer::TokenType::Return)) {
    return parseReturnStmt();
  } else if (match(lexer::TokenType::For)) {
    return parseForStmt();
  } else if (match(lexer::TokenType::Comptime)) {
    return parseComptimeStmt();
  } else if (match(lexer::TokenType::Break)) {
    expect(lexer::TokenType::Semicolon, "Expected ';' after break");
    return std::make_unique<ast::BreakStmt>();
  } else if (match(lexer::TokenType::Continue)) {
    expect(lexer::TokenType::Semicolon, "Expected ';' after continue");
    return std::make_unique<ast::ContinueStmt>();
  } else if (match(lexer::TokenType::Goto)) {
    if (!check(lexer::TokenType::Identifier)) {
      error("Expected label name after goto");
      return nullptr;
    }
    std::string label = currentToken->getValue();
    advance();
    expect(lexer::TokenType::Semicolon, "Expected ';' after goto label");
    return std::make_unique<ast::GotoStmt>(std::move(label));
  } else if (match(lexer::TokenType::Try)) {
    return parseTryStmt();
  } else if (match(lexer::TokenType::Throw)) {
    return parseThrowStmt();
  } else if (match(lexer::TokenType::Defer)) {
    return parseDeferStmt();
  } else if (check(lexer::TokenType::Identifier)) {
    // 检查是否是标签（后面跟着冒号）
    auto state = saveState();
    std::string label = currentToken->getValue();
    advance();
    if (match(lexer::TokenType::Colon)) {
      return std::make_unique<ast::LabelStmt>(std::move(label));
    }
    // 不是标签，回滚并解析表达式语句
    restoreState(state);
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
    return std::make_unique<ast::VariableStmt>(std::move(varDecl));
  } else if (check(lexer::TokenType::Var) || check(lexer::TokenType::Let) ||
             check(lexer::TokenType::Late) || isTypeStart()) {
    auto varDecl = tryParseVariableDecl();
    if (varDecl) {
      return std::make_unique<ast::VariableStmt>(std::move(varDecl));
    }
  }

  // 默认解析表达式语句
  return parseExprStmt();
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

// 解析复合语句
std::unique_ptr<ast::CompoundStmt> Parser::parseCompoundStmt() {
  expect(lexer::TokenType::LBrace, "Expected '{'");

  std::vector<std::unique_ptr<ast::Node>> statements;
  while (!check(lexer::TokenType::RBrace) &&
         !check(lexer::TokenType::EndOfFile)) {
    statements.push_back(parseStatement());
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
  } else if (match(lexer::TokenType::At)) {
    auto expr = parseUnaryExpr();
    return std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::At,
                                            std::move(expr));
  }

  return parsePrimaryExpr();
}

// 解析主表达式
std::unique_ptr<ast::Expression> Parser::parsePrimaryExpr() {
  std::unique_ptr<ast::Expression> expr;

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
    // 如果下一个 token 是 [、字面量或 ]，则是数组初始化
    if (check(lexer::TokenType::LBracket) ||
        check(lexer::TokenType::IntegerLiteral) ||
        check(lexer::TokenType::FloatingLiteral) ||
        check(lexer::TokenType::StringLiteral) ||
        check(lexer::TokenType::BooleanLiteral) ||
        check(lexer::TokenType::CharacterLiteral) ||
        check(lexer::TokenType::Null) || check(lexer::TokenType::RBracket)) {
      expr = parseArrayInit();
    } else {
      expr = parseLambdaOrArrayInit();
    }
  } else if (match(lexer::TokenType::LBrace)) {
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
      expr = std::make_unique<ast::UnaryExpr>(ast::UnaryExpr::Op::Immutable,
                                              std::move(expr));
    } else if (match(lexer::TokenType::Range)) {
      expr = parseRangeExpr(std::move(expr));
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
  return std::make_unique<ast::MemberExpr>(std::move(object), member);
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
  return std::make_unique<ast::MemberExpr>(std::move(object), member);
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
std::unique_ptr<ast::Expression> Parser::parseStructInit() {
  std::vector<std::pair<std::string, std::unique_ptr<ast::Expression>>> fields;

  if (!check(lexer::TokenType::RBrace)) {
    do {
      if (!check(lexer::TokenType::Identifier)) {
        error("Expected identifier in struct initializer");
        return nullptr;
      }
      std::string name = currentToken->getValue();
      advance();

      expect(lexer::TokenType::Colon, "Expected ':' after field name");
      auto value = parseExpression();
      fields.push_back({name, std::move(value)});
    } while (match(lexer::TokenType::Comma));
  }

  expect(lexer::TokenType::RBrace, "Expected '}' after struct fields");
  return std::make_unique<ast::StructInitExpr>(std::move(fields));
}

// 解析类型
std::unique_ptr<ast::Type> Parser::parseType() {
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

    expect(lexer::TokenType::Arrow, "Expected '->' after function parameters");

    auto returnType = parseType();
    return std::make_unique<ast::FunctionType>(std::move(parameterTypes),
                                               std::move(returnType));
  }

  if (!check(lexer::TokenType::Identifier) && !isTypeKeyword()) {
    error("Expected type name");
    return nullptr;
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
  } else if (name == "int") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Int);
  } else if (name == "uint") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::UInt);
  } else if (name == "long") {
    type = std::make_unique<ast::PrimitiveType>(ast::PrimitiveType::Kind::Long);
  } else if (name == "ulong") {
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
    type = std::make_unique<ast::NamedType>("LiteralView");
  } else {
    type = std::make_unique<ast::NamedType>(name);
  }

  // 检查是否是只读类型
  if (match(lexer::TokenType::Not)) {
    type = std::make_unique<ast::ReadonlyType>(std::move(type));
  }

  return parseTypeSuffix(std::move(type));
}

// 解析类型后缀（指针、引用、数组、切片、泛型）
std::unique_ptr<ast::Type>
Parser::parseTypeSuffix(std::unique_ptr<ast::Type> type) {
  while (true) {
    if (match(lexer::TokenType::AddressOf)) {
      type = std::make_unique<ast::PointerType>(std::move(type), false);
    } else if (match(lexer::TokenType::And)) {
      type = std::make_unique<ast::ReferenceType>(std::move(type));
    } else if (match(lexer::TokenType::LBracket)) {
      // 解析 [ ... ] 中的内容，可能是多维的
      if (match(lexer::TokenType::RBracket)) {
        // 单个 [] → 一维切片
        type = std::make_unique<ast::SliceType>(std::move(type));
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
    } else if (match(lexer::TokenType::Lt)) {
      auto args = parseTemplateArguments();
      expect(lexer::TokenType::Gt, "Expected '>' after template arguments");
      type =
          std::make_unique<ast::GenericType>(type->toString(), std::move(args));
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
      params.push_back(parseParameter());
    } while (match(lexer::TokenType::Comma));
  }

  return params;
}

// 解析参数
std::unique_ptr<ast::Node> Parser::parseParameter() {
  bool isVariadic = false;

  if (match(lexer::TokenType::Ellipsis)) {
    isVariadic = true;
  }

  if (!check(lexer::TokenType::Identifier) && !isTypeKeyword()) {
    error("Expected parameter type");
    return nullptr;
  }

  auto type = parseType();

  if (!check(lexer::TokenType::Identifier)) {
    error("Expected parameter name");
    return nullptr;
  }

  std::string name = currentToken->getValue();
  advance();

  std::unique_ptr<ast::Expression> defaultValue;
  if (match(lexer::TokenType::Assign)) {
    defaultValue = parseExpression();
  }

  return std::make_unique<ast::Parameter>(name, std::move(type),
                                          std::move(defaultValue));
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
  if (!check(lexer::TokenType::Identifier)) {
    error("Expected constraint in where clause");
    return nullptr;
  }

  auto constraint = parseType();
  return constraint;
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
         currentToken->getType() == lexer::TokenType::Identifier;
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
         type == lexer::TokenType::Char || type == lexer::TokenType::Func ||
         type == lexer::TokenType::LiteralView;
}

// 其他TODO函数
std::unique_ptr<ast::Node> Parser::parseForInit() {
  if (check(lexer::TokenType::Semicolon)) {
    advance();
    return nullptr;
  }

  if (check(lexer::TokenType::Var) || check(lexer::TokenType::Let) ||
      check(lexer::TokenType::Late)) {
    return parseVariableDecl();
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
  if (match(lexer::TokenType::Identifier)) {
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::IntegerLiteral)) {
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::StringLiteral)) {
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::BooleanLiteral)) {
    return std::make_unique<ast::Pattern>();
  } else if (match(lexer::TokenType::Null)) {
    return std::make_unique<ast::Pattern>();
  } else if (check(lexer::TokenType::Identifier) &&
             currentToken->getValue() == "_") {
    advance();
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

  expect(lexer::TokenType::LBrace, "Expected '{' after extern ABI");

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
