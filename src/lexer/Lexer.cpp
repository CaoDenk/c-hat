#include "Lexer.h"
#include <cctype>
#include <unordered_map>

namespace c_hat {
namespace lexer {

// 关键字映射
static std::unordered_map<std::string, TokenType> keywordMap = {
    {"func", TokenType::Func},
    {"class", TokenType::Class},
    {"struct", TokenType::Struct},
    {"enum", TokenType::Enum},
    {"union", TokenType::Union},
    {"interface", TokenType::Interface},
    {"module", TokenType::Module},
    {"import", TokenType::Import},
    {"export", TokenType::Export},
    {"extension", TokenType::Extension},
    {"get", TokenType::Get},
    {"set", TokenType::Set},
    {"public", TokenType::Public},
    {"private", TokenType::Private},
    {"protected", TokenType::Protected},
    {"internal", TokenType::Internal},
    {"static", TokenType::Static},
    {"const", TokenType::Const},
    {"virtual", TokenType::Virtual},
    {"override", TokenType::Override},
    {"final", TokenType::Final},
    {"abstract", TokenType::Abstract},
    {"inline", TokenType::Inline},
    {"using", TokenType::Using},
    {"try", TokenType::Try},
    {"catch", TokenType::Catch},
    {"throw", TokenType::Throw},
    {"defer", TokenType::Defer},
    {"await", TokenType::Await},
    {"yield", TokenType::Yield},
    {"comptime", TokenType::Comptime},
    {"late", TokenType::Late},
    {"var", TokenType::Var},
    {"let", TokenType::Let},
    {"void", TokenType::Void},
    {"bool", TokenType::Bool},
    {"byte", TokenType::Byte},
    {"sbyte", TokenType::SByte},
    {"short", TokenType::Short},
    {"ushort", TokenType::UShort},
    {"int", TokenType::Int},
    {"int32", TokenType::Int},
    {"uint", TokenType::UInt},
    {"uint32", TokenType::UInt},
    {"long", TokenType::Long},
    {"int64", TokenType::Long},
    {"ulong", TokenType::ULong},
    {"uint64", TokenType::ULong},
    {"float", TokenType::Float},
    {"double", TokenType::Double},
    {"fp16", TokenType::Fp16},
    {"bf16", TokenType::Bf16},
    {"char", TokenType::Char},
    {"true", TokenType::True},
    {"false", TokenType::False},
    {"null", TokenType::Null},
    {"self", TokenType::Self},
    {"base", TokenType::Base},
    {"new", TokenType::New},
    {"delete", TokenType::Delete},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"match", TokenType::Match},
    {"case", TokenType::Case},
    {"default", TokenType::Default},
    {"for", TokenType::For},
    {"while", TokenType::While},
    {"do", TokenType::Do},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"return", TokenType::Return},
    {"goto", TokenType::Goto},
    {"as", TokenType::As},
    {"where", TokenType::Where},
    {"requires", TokenType::Requires},
    {"concept", TokenType::Concept},
    {"implicit", TokenType::Implicit},
    {"operator", TokenType::Operator},
    {"mutable", TokenType::Mutable},
    {"sizeof", TokenType::Sizeof},
    {"typeof", TokenType::Typeof},
    {"extern", TokenType::Extern},
    {"literalview", TokenType::LiteralView}};

// Token 转字符串
std::string Token::toString() const {
  std::string typeStr;
  switch (type) {
  case TokenType::Func:
    typeStr = "Func";
    break;
  case TokenType::Class:
    typeStr = "Class";
    break;
  case TokenType::Struct:
    typeStr = "Struct";
    break;
  case TokenType::Enum:
    typeStr = "Enum";
    break;
  case TokenType::Union:
    typeStr = "Union";
    break;
  case TokenType::Interface:
    typeStr = "Interface";
    break;
  case TokenType::Module:
    typeStr = "Module";
    break;
  case TokenType::Import:
    typeStr = "Import";
    break;
  case TokenType::Extension:
    typeStr = "Extension";
    break;
  case TokenType::Get:
    typeStr = "Get";
    break;
  case TokenType::Set:
    typeStr = "Set";
    break;
  case TokenType::Public:
    typeStr = "Public";
    break;
  case TokenType::Private:
    typeStr = "Private";
    break;
  case TokenType::Protected:
    typeStr = "Protected";
    break;
  case TokenType::Internal:
    typeStr = "Internal";
    break;
  case TokenType::Static:
    typeStr = "Static";
    break;
  case TokenType::Const:
    typeStr = "Const";
    break;
  case TokenType::Virtual:
    typeStr = "Virtual";
    break;
  case TokenType::Override:
    typeStr = "Override";
    break;
  case TokenType::Final:
    typeStr = "Final";
    break;
  case TokenType::Abstract:
    typeStr = "Abstract";
    break;
  case TokenType::Inline:
    typeStr = "Inline";
    break;
  case TokenType::Using:
    typeStr = "Using";
    break;
  case TokenType::Try:
    typeStr = "Try";
    break;
  case TokenType::Catch:
    typeStr = "Catch";
    break;
  case TokenType::Throw:
    typeStr = "Throw";
    break;
  case TokenType::Defer:
    typeStr = "Defer";
    break;
  case TokenType::Await:
    typeStr = "Await";
    break;
  case TokenType::Yield:
    typeStr = "Yield";
    break;
  case TokenType::Comptime:
    typeStr = "Comptime";
    break;
  case TokenType::Late:
    typeStr = "Late";
    break;
  case TokenType::Var:
    typeStr = "Var";
    break;
  case TokenType::Let:
    typeStr = "Let";
    break;
  case TokenType::Void:
    typeStr = "Void";
    break;
  case TokenType::Bool:
    typeStr = "Bool";
    break;
  case TokenType::Byte:
    typeStr = "Byte";
    break;
  case TokenType::SByte:
    typeStr = "SByte";
    break;
  case TokenType::Short:
    typeStr = "Short";
    break;
  case TokenType::UShort:
    typeStr = "UShort";
    break;
  case TokenType::Int:
    typeStr = "Int";
    break;
  case TokenType::UInt:
    typeStr = "UInt";
    break;
  case TokenType::Long:
    typeStr = "Long";
    break;
  case TokenType::ULong:
    typeStr = "ULong";
    break;
  case TokenType::Float:
    typeStr = "Float";
    break;
  case TokenType::Double:
    typeStr = "Double";
    break;
  case TokenType::Fp16:
    typeStr = "Fp16";
    break;
  case TokenType::Bf16:
    typeStr = "Bf16";
    break;
  case TokenType::Char:
    typeStr = "Char";
    break;
  case TokenType::True:
    typeStr = "True";
    break;
  case TokenType::False:
    typeStr = "False";
    break;
  case TokenType::Null:
    typeStr = "Null";
    break;
  case TokenType::Self:
    typeStr = "Self";
    break;
  case TokenType::Base:
    typeStr = "Base";
    break;
  case TokenType::New:
    typeStr = "New";
    break;
  case TokenType::Delete:
    typeStr = "Delete";
    break;
  case TokenType::If:
    typeStr = "If";
    break;
  case TokenType::Else:
    typeStr = "Else";
    break;
  case TokenType::Match:
    typeStr = "Match";
    break;
  case TokenType::Case:
    typeStr = "Case";
    break;
  case TokenType::Default:
    typeStr = "Default";
    break;
  case TokenType::For:
    typeStr = "For";
    break;
  case TokenType::While:
    typeStr = "While";
    break;
  case TokenType::Do:
    typeStr = "Do";
    break;
  case TokenType::Break:
    typeStr = "Break";
    break;
  case TokenType::Continue:
    typeStr = "Continue";
    break;
  case TokenType::Return:
    typeStr = "Return";
    break;
  case TokenType::Goto:
    typeStr = "Goto";
    break;
  case TokenType::As:
    typeStr = "As";
    break;
  case TokenType::Where:
    typeStr = "Where";
    break;
  case TokenType::Requires:
    typeStr = "Requires";
    break;
  case TokenType::Concept:
    typeStr = "Concept";
    break;
  case TokenType::Operator:
    typeStr = "Operator";
    break;
  case TokenType::Mutable:
    typeStr = "Mutable";
    break;
  case TokenType::Sizeof:
    typeStr = "Sizeof";
    break;
  case TokenType::Typeof:
    typeStr = "Typeof";
    break;
  case TokenType::Extern:
    typeStr = "Extern";
    break;
  case TokenType::LiteralView:
    typeStr = "LiteralView";
    break;
  case TokenType::Identifier:
    typeStr = "Identifier";
    break;
  case TokenType::IntegerLiteral:
    typeStr = "IntegerLiteral";
    break;
  case TokenType::FloatingLiteral:
    typeStr = "FloatingLiteral";
    break;
  case TokenType::CharacterLiteral:
    typeStr = "CharacterLiteral";
    break;
  case TokenType::StringLiteral:
    typeStr = "StringLiteral";
    break;
  case TokenType::BooleanLiteral:
    typeStr = "BooleanLiteral";
    break;
  case TokenType::Plus:
    typeStr = "Plus";
    break;
  case TokenType::Minus:
    typeStr = "Minus";
    break;
  case TokenType::Multiply:
    typeStr = "Multiply";
    break;
  case TokenType::Divide:
    typeStr = "Divide";
    break;
  case TokenType::Modulus:
    typeStr = "Modulus";
    break;
  case TokenType::Power:
    typeStr = "Power";
    break;
  case TokenType::And:
    typeStr = "And";
    break;
  case TokenType::Or:
    typeStr = "Or";
    break;
  case TokenType::Xor:
    typeStr = "Xor";
    break;
  case TokenType::Not:
    typeStr = "Not";
    break;
  case TokenType::BitNot:
    typeStr = "BitNot";
    break;
  case TokenType::Shl:
    typeStr = "Shl";
    break;
  case TokenType::Shr:
    typeStr = "Shr";
    break;
  case TokenType::Lt:
    typeStr = "Lt";
    break;
  case TokenType::Le:
    typeStr = "Le";
    break;
  case TokenType::Gt:
    typeStr = "Gt";
    break;
  case TokenType::Ge:
    typeStr = "Ge";
    break;
  case TokenType::Eq:
    typeStr = "Eq";
    break;
  case TokenType::Ne:
    typeStr = "Ne";
    break;
  case TokenType::LogicAnd:
    typeStr = "LogicAnd";
    break;
  case TokenType::LogicOr:
    typeStr = "LogicOr";
    break;
  case TokenType::Assign:
    typeStr = "Assign";
    break;
  case TokenType::AddAssign:
    typeStr = "AddAssign";
    break;
  case TokenType::SubAssign:
    typeStr = "SubAssign";
    break;
  case TokenType::MulAssign:
    typeStr = "MulAssign";
    break;
  case TokenType::DivAssign:
    typeStr = "DivAssign";
    break;
  case TokenType::ModAssign:
    typeStr = "ModAssign";
    break;
  case TokenType::AndAssign:
    typeStr = "AndAssign";
    break;
  case TokenType::OrAssign:
    typeStr = "OrAssign";
    break;
  case TokenType::XorAssign:
    typeStr = "XorAssign";
    break;
  case TokenType::ShlAssign:
    typeStr = "ShlAssign";
    break;
  case TokenType::ShrAssign:
    typeStr = "ShrAssign";
    break;
  case TokenType::AddressOf:
    typeStr = "AddressOf";
    break;
  case TokenType::Dereference:
    typeStr = "Dereference";
    break;
  case TokenType::Question:
    typeStr = "Question";
    break;
  case TokenType::Tilde:
    typeStr = "Tilde";
    break;
  case TokenType::Arrow:
    typeStr = "Arrow";
    break;
  case TokenType::LParen:
    typeStr = "LParen";
    break;
  case TokenType::RParen:
    typeStr = "RParen";
    break;
  case TokenType::LBrace:
    typeStr = "LBrace";
    break;
  case TokenType::RBrace:
    typeStr = "RBrace";
    break;
  case TokenType::LBracket:
    typeStr = "LBracket";
    break;
  case TokenType::RBracket:
    typeStr = "RBracket";
    break;
  case TokenType::Dot:
    typeStr = "Dot";
    break;
  case TokenType::Comma:
    typeStr = "Comma";
    break;
  case TokenType::Semicolon:
    typeStr = "Semicolon";
    break;
  case TokenType::Colon:
    typeStr = "Colon";
    break;
  case TokenType::DoubleColon:
    typeStr = "DoubleColon";
    break;
  case TokenType::Ellipsis:
    typeStr = "Ellipsis";
    break;
  case TokenType::Range:
    typeStr = "Range";
    break;
  case TokenType::Dollar:
    typeStr = "Dollar";
    break;
  case TokenType::Bar:
    typeStr = "Bar";
    break;
  case TokenType::FatArrow:
    typeStr = "FatArrow";
    break;
  case TokenType::At:
    typeStr = "At";
    break;
  case TokenType::LineComment:
    typeStr = "LineComment";
    break;
  case TokenType::BlockComment:
    typeStr = "BlockComment";
    break;
  case TokenType::Whitespace:
    typeStr = "Whitespace";
    break;
  case TokenType::EndOfFile:
    typeStr = "EndOfFile";
    break;
  default:
    typeStr = "Unknown";
    break;
  }
  return typeStr + "(\"" + value + "\")";
}

// Lexer 构造函数
Lexer::Lexer(std::string source)
    : source(std::move(source)), position(0), line(1), column(1) {}

// 获取下一个词法单元
std::optional<Token> Lexer::nextToken() {
  // 检查是否有回溯的 token
  if (!ungetTokens.empty()) {
    Token token = ungetTokens.back();
    ungetTokens.pop_back();
    return token;
  }

  // 跳过空白
  skipWhitespace();

  // 检查是否到达文件末尾
  if (isEOF()) {
    return Token(TokenType::EndOfFile, "", line, column);
  }

  char c = currentChar();

  // 处理标识符和关键字
  if (std::isalpha(c) || c == '_') {
    return processIdentifier();
  }

  // 处理数字字面量
  if (std::isdigit(c)) {
    return processNumber();
  }

  // 处理字符字面量
  if (c == '\'') {
    return processCharacter();
  }

  // 处理字符串字面量
  if (c == '"') {
    return processString();
  }

  // 处理注释
  if (c == '/') {
    return processComment();
  }

  // 处理操作符和特殊符号
  return processOperator();
}

// 回溯一个词法单元
void Lexer::ungetToken(const Token &token) { ungetTokens.push_back(token); }

// 跳过空白字符
void Lexer::skipWhitespace() {
  while (!isEOF()) {
    char c = currentChar();
    if (std::isspace(c)) {
      if (c == '\n') {
        line++;
        column = 1;
      } else {
        column++;
      }
      advance();
    } else if (c == '/' && peekChar() == '/') {
      // 跳过行注释
      advance(2);
      while (!isEOF() && currentChar() != '\n') {
        advance();
      }
    } else if (c == '/' && peekChar() == '*') {
      // 跳过多行注释
      advance(2);
      int startLine = line;
      while (!isEOF() && !(currentChar() == '*' && peekChar() == '/')) {
        if (currentChar() == '\n') {
          line++;
          column = 1;
        } else {
          column++;
        }
        advance();
      }
      if (!isEOF()) {
        advance(2);
        column += 2;
      }
    } else {
      break;
    }
  }
}

// 处理标识符和关键字
std::optional<Token> Lexer::processIdentifier() {
  int startLine = line;
  int startColumn = column;
  std::string identifier;

  while (!isEOF()) {
    char c = currentChar();
    if (std::isalnum(c) || c == '_') {
      identifier += c;
      advance();
      column++;
    } else {
      break;
    }
  }

  // 检查是否为关键字
  auto it = keywordMap.find(identifier);
  if (it != keywordMap.end()) {
    return Token(it->second, identifier, startLine, startColumn);
  }

  return Token(TokenType::Identifier, identifier, startLine, startColumn);
}

// 处理数字字面量
std::optional<Token> Lexer::processNumber() {
  int startLine = line;
  int startColumn = column;
  std::string number;
  bool isFloat = false;

  // 处理整数部分
  while (!isEOF() && std::isdigit(currentChar())) {
    number += currentChar();
    advance();
    column++;
  }

  // 处理小数部分
  if (!isEOF() && currentChar() == '.') {
    isFloat = true;
    number += '.';
    advance();
    column++;

    while (!isEOF() && std::isdigit(currentChar())) {
      number += currentChar();
      advance();
      column++;
    }
  }

  // 处理指数部分
  if (!isEOF() && (currentChar() == 'e' || currentChar() == 'E')) {
    isFloat = true;
    number += currentChar();
    advance();
    column++;

    if (!isEOF() && (currentChar() == '+' || currentChar() == '-')) {
      number += currentChar();
      advance();
      column++;
    }

    while (!isEOF() && std::isdigit(currentChar())) {
      number += currentChar();
      advance();
      column++;
    }
  }

  // 处理后缀
  if (!isEOF() && std::isalpha(currentChar())) {
    char suffix = currentChar();
    number += suffix;
    advance();
    column++;
    isFloat =
        (suffix == 'f' || suffix == 'F' || suffix == 'd' || suffix == 'D');
  }

  if (isFloat) {
    return Token(TokenType::FloatingLiteral, number, startLine, startColumn);
  } else {
    return Token(TokenType::IntegerLiteral, number, startLine, startColumn);
  }
}

// 处理字符字面量
std::optional<Token> Lexer::processCharacter() {
  int startLine = line;
  int startColumn = column;
  std::string literal = "'";
  advance();
  column++;

  // 处理转义字符
  if (!isEOF() && currentChar() == '\\') {
    literal += '\\';
    advance();
    column++;

    if (!isEOF()) {
      literal += currentChar();
      advance();
      column++;
    }
  } else if (!isEOF() && currentChar() != '\'') {
    literal += currentChar();
    advance();
    column++;
  }

  if (!isEOF() && currentChar() == '\'') {
    literal += "'";
    advance();
    column++;
    return Token(TokenType::CharacterLiteral, literal, startLine, startColumn);
  } else {
    // 错误：未闭合的字符字面量
    return std::nullopt;
  }
}

// 处理字符串字面量
std::optional<Token> Lexer::processString() {
  int startLine = line;
  int startColumn = column;
  std::string literal = "\"";
  advance();
  column++;

  while (!isEOF() && currentChar() != '"') {
    // 处理转义字符
    if (currentChar() == '\\') {
      literal += '\\';
      advance();
      column++;

      if (!isEOF()) {
        literal += currentChar();
        advance();
        column++;
      }
    } else {
      literal += currentChar();
      advance();
      column++;
    }
  }

  if (!isEOF() && currentChar() == '"') {
    literal += '"';
    advance();
    column++;
    return Token(TokenType::StringLiteral, literal, startLine, startColumn);
  } else {
    // 错误：未闭合的字符串字面量
    return std::nullopt;
  }
}

// 处理注释
std::optional<Token> Lexer::processComment() {
  int startLine = line;
  int startColumn = column;

  if (peekChar() == '/') {
    // 行注释
    std::string comment = "//";
    advance(2);
    column += 2;

    while (!isEOF() && currentChar() != '\n') {
      comment += currentChar();
      advance();
      column++;
    }

    return Token(TokenType::LineComment, comment, startLine, startColumn);
  } else if (peekChar() == '*') {
    // 块注释
    std::string comment = "/*";
    advance(2);
    column += 2;

    while (!isEOF() && !(currentChar() == '*' && peekChar() == '/')) {
      if (currentChar() == '\n') {
        line++;
        column = 1;
      } else {
        column++;
      }
      comment += currentChar();
      advance();
    }

    if (!isEOF()) {
      comment += "*/";
      advance(2);
      column += 2;
    }

    return Token(TokenType::BlockComment, comment, startLine, startColumn);
  } else {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::DivAssign, "/=", startLine, startColumn);
    }
    return Token(TokenType::Divide, "/", startLine, startColumn);
  }
}

// 处理操作符和特殊符号
std::optional<Token> Lexer::processOperator() {
  int startLine = line;
  int startColumn = column;
  char c = currentChar();

  switch (c) {
  case '+': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::AddAssign, "+=", startLine, startColumn);
    }
    return Token(TokenType::Plus, "+", startLine, startColumn);
  }
  case '-': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::SubAssign, "-=", startLine, startColumn);
    } else if (!isEOF() && currentChar() == '>') {
      advance();
      column++;
      return Token(TokenType::Arrow, "->", startLine, startColumn);
    }
    return Token(TokenType::Minus, "-", startLine, startColumn);
  }
  case '*': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '*') {
      advance();
      column++;
      return Token(TokenType::Power, "**", startLine, startColumn);
    } else if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::MulAssign, "*=", startLine, startColumn);
    }
    return Token(TokenType::Multiply, "*", startLine, startColumn);
  }
  case '/': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::DivAssign, "/=", startLine, startColumn);
    }
    return Token(TokenType::Divide, "/", startLine, startColumn);
  }
  case '%': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::ModAssign, "%=", startLine, startColumn);
    }
    return Token(TokenType::Modulus, "%", startLine, startColumn);
  }
  case '^': {
    advance();
    column++;
    // 注意：这里需要根据上下文判断是 AddressOf 还是 Dereference
    // 暂时都返回 AddressOf，由解析器根据上下文判断
    return Token(TokenType::AddressOf, "^", startLine, startColumn);
  }
  case '&': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::AndAssign, "&=", startLine, startColumn);
    } else if (!isEOF() && currentChar() == '&') {
      advance();
      column++;
      return Token(TokenType::LogicAnd, "&&", startLine, startColumn);
    }
    return Token(TokenType::And, "&", startLine, startColumn);
  }
  case '|': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::OrAssign, "|=", startLine, startColumn);
    } else if (!isEOF() && currentChar() == '|') {
      advance();
      column++;
      return Token(TokenType::LogicOr, "||", startLine, startColumn);
    }
    return Token(TokenType::Bar, "|", startLine, startColumn);
  }
  case '!': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::Ne, "!=", startLine, startColumn);
    }
    return Token(TokenType::Not, "!", startLine, startColumn);
  }
  case '~': {
    advance();
    column++;
    return Token(TokenType::Tilde, "~", startLine, startColumn);
  }
  case '<': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::Le, "<=", startLine, startColumn);
    } else if (!isEOF() && currentChar() == '<') {
      advance();
      column++;
      if (!isEOF() && currentChar() == '=') {
        advance();
        column++;
        return Token(TokenType::ShlAssign, "<<=", startLine, startColumn);
      }
      return Token(TokenType::Shl, "<<", startLine, startColumn);
    }
    return Token(TokenType::Lt, "<", startLine, startColumn);
  }
  case '>': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::Ge, ">=", startLine, startColumn);
    } else if (!isEOF() && currentChar() == '>') {
      advance();
      column++;
      if (!isEOF() && currentChar() == '=') {
        advance();
        column++;
        return Token(TokenType::ShrAssign, ">>=", startLine, startColumn);
      }
      return Token(TokenType::Shr, ">>", startLine, startColumn);
    }
    return Token(TokenType::Gt, ">", startLine, startColumn);
  }
  case '=': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '>') {
      advance();
      column++;
      return Token(TokenType::FatArrow, "=>", startLine, startColumn);
    } else if (!isEOF() && currentChar() == '=') {
      advance();
      column++;
      return Token(TokenType::Eq, "==", startLine, startColumn);
    }
    return Token(TokenType::Assign, "=", startLine, startColumn);
  }
  case '(': {
    advance();
    column++;
    return Token(TokenType::LParen, "(", startLine, startColumn);
  }
  case ')': {
    advance();
    column++;
    return Token(TokenType::RParen, ")", startLine, startColumn);
  }
  case '{': {
    advance();
    column++;
    return Token(TokenType::LBrace, "{", startLine, startColumn);
  }
  case '}': {
    advance();
    column++;
    return Token(TokenType::RBrace, "}", startLine, startColumn);
  }
  case '[': {
    advance();
    column++;
    return Token(TokenType::LBracket, "[", startLine, startColumn);
  }
  case ']': {
    advance();
    column++;
    return Token(TokenType::RBracket, "]", startLine, startColumn);
  }
  case '.': {
    advance();
    column++;
    if (!isEOF() && currentChar() == '.') {
      advance();
      column++;
      if (!isEOF() && currentChar() == '.') {
        advance();
        column++;
        return Token(TokenType::Ellipsis, "...", startLine, startColumn);
      }
      return Token(TokenType::Range, "..", startLine, startColumn);
    }
    return Token(TokenType::Dot, ".", startLine, startColumn);
  }
  case ',': {
    advance();
    column++;
    return Token(TokenType::Comma, ",", startLine, startColumn);
  }
  case ';': {
    advance();
    column++;
    return Token(TokenType::Semicolon, ";", startLine, startColumn);
  }
  case ':': {
    advance();
    column++;
    if (!isEOF() && currentChar() == ':') {
      advance();
      column++;
      return Token(TokenType::DoubleColon, "::", startLine, startColumn);
    }
    return Token(TokenType::Colon, ":", startLine, startColumn);
  }
  case '?': {
    advance();
    column++;
    return Token(TokenType::Question, "?", startLine, startColumn);
  }
  case '$': {
    advance();
    column++;
    return Token(TokenType::Dollar, "$", startLine, startColumn);
  }
  case '@': {
    advance();
    column++;
    return Token(TokenType::At, "@", startLine, startColumn);
  }
  default: {
    // 未知字符
    advance();
    column++;
    return Token(TokenType::Identifier, std::string(1, c), startLine,
                 startColumn);
  }
  }
}

// 当前位置
char Lexer::currentChar() const {
  if (position < source.length()) {
    return source[position];
  }
  return '\0';
}

// 前进一个字符
void Lexer::advance() {
  if (position < source.length()) {
    position++;
  }
}

// 前进多个字符
void Lexer::advance(int count) {
  position += count;
  if (position > source.length()) {
    position = source.length();
  }
}

// 查看下一个字符
char Lexer::peekChar() const {
  if (position + 1 < source.length()) {
    return source[position + 1];
  }
  return '\0';
}

// 查看多个字符
std::string Lexer::peekChars(int count) const {
  std::string result;
  for (int i = 1; i <= count && position + i < source.length(); i++) {
    result += source[position + i];
  }
  return result;
}

// 检查是否到达文件末尾
bool Lexer::isEOF() const { return position >= source.length(); }

// 保存词法分析器状态
Lexer::LexerState Lexer::saveState() {
  return LexerState{position, line, column, ungetTokens};
}

// 恢复词法分析器状态
void Lexer::restoreState(const LexerState &state) {
  position = state.position;
  line = state.line;
  column = state.column;
  ungetTokens = state.ungetTokens;
}

} // namespace lexer
} // namespace c_hat
