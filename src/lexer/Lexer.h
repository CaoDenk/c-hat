#pragma once

#include "TokenType.h"
#include <optional>
#include <string>
#include <vector>

namespace c_hat {
namespace lexer {

// 词法单元
class Token {
public:
  Token(TokenType type, std::string value, int line, int column)
      : type(type), value(std::move(value)), line(line), column(column) {}

  TokenType getType() const { return type; }
  const std::string &getValue() const { return value; }
  int getLine() const { return line; }
  int getColumn() const { return column; }

  std::string toString() const;

private:
  TokenType type;
  std::string value;
  int line;
  int column;
};

// 词法分析器
class Lexer {
public:
  Lexer(std::string source);

  // 获取下一个词法单元
  std::optional<Token> nextToken();

  // 回溯一个词法单元
  void ungetToken(const Token &token);

  // 保存和恢复词法分析器状态
  struct LexerState {
    size_t position;
    int line;
    int column;
    std::vector<Token> ungetTokens;
  };

  LexerState saveState();
  void restoreState(const LexerState &state);

private:
  // 跳过空白字符
  void skipWhitespace();

  // 处理标识符和关键字
  std::optional<Token> processIdentifier();

  // 处理数字字面量
  std::optional<Token> processNumber();

  // 处理字符字面量
  std::optional<Token> processCharacter();

  // 处理字符串字面量
  std::optional<Token> processString();

  // 处理注释
  std::optional<Token> processComment();

  // 处理操作符和特殊符号
  std::optional<Token> processOperator();

  // 检查是否为关键字
  TokenType checkKeyword(const std::string &identifier) const;

  // 当前位置
  char currentChar() const;

  // 前进一个字符
  void advance();

  // 前进多个字符
  void advance(int count);

  // 查看下一个字符
  char peekChar() const;

  // 查看多个字符
  std::string peekChars(int count) const;

  // 检查是否到达文件末尾
  bool isEOF() const;

  std::string source;
  size_t position;
  int line;
  int column;
  std::vector<Token> ungetTokens;
};

} // namespace lexer
} // namespace c_hat
