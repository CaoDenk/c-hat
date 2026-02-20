#pragma once

#include "../ast/AstNodes.h"
#include "../lexer/Lexer.h"
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

namespace c_hat {
namespace parser {

// 语法分析器
class Parser {
public:
  Parser(std::string source);

  // 解析整个程序
  std::unique_ptr<ast::Program> parseProgram();

private:
  // 词法分析器
  lexer::Lexer lexer;

  // 当前词法单元
  std::optional<lexer::Token> currentToken;
  // 前一个词法单元
  std::optional<lexer::Token> previousToken;

  // 错误处理
  void error(const std::string &message);

  // 消费下一个词法单元
  void advance();

  // 检查当前词法单元类型
  bool check(lexer::TokenType type) const;

  // 匹配并消费指定类型的词法单元
  bool match(lexer::TokenType type);

  // 期望并消费指定类型的词法单元
  void expect(lexer::TokenType type, const std::string &message);

  // 解析声明
  std::unique_ptr<ast::Declaration> parseDeclaration();

  // 解析变量声明
  std::unique_ptr<ast::VariableDecl> parseVariableDecl();

  // 解析元组解构声明
  std::unique_ptr<ast::TupleDestructuringDecl>
  parseTupleDestructuringDecl(const std::string &specifiers, bool isLate,
                              ast::VariableKind kind);

  // 尝试解析变量声明（失败时返回 nullptr，不抛异常）
  std::unique_ptr<ast::VariableDecl> tryParseVariableDecl();

  // 解析函数声明
  std::unique_ptr<ast::FunctionDecl> parseFunctionDecl();

  // 解析类声明
  std::unique_ptr<ast::ClassDecl> parseClassDecl();

  // 解析结构体声明
  std::unique_ptr<ast::StructDecl> parseStructDecl();

  // 解析枚举声明
  std::unique_ptr<ast::EnumDecl> parseEnumDecl();

  // 解析枚举成员
  std::unique_ptr<ast::EnumMember> parseEnumMember();

  // 解析模块声明
  std::unique_ptr<ast::Declaration> parseModuleDecl();

  // 解析导入声明
  std::unique_ptr<ast::Declaration>
  parseImportDecl(const std::string &specifiers);

  // 解析扩展声明
  std::unique_ptr<ast::Declaration> parseExtensionDecl();

  // 解析 Getter 声明
  std::unique_ptr<ast::GetterDecl> parseGetterDecl();

  // 解析 Setter 声明
  std::unique_ptr<ast::SetterDecl> parseSetterDecl();

  // 解析类型别名声明
  std::unique_ptr<ast::TypeAliasDecl> parseTypeAliasDecl();

  // 解析外部声明块
  std::unique_ptr<ast::Declaration> parseExternDecl();

  // 解析语句
  std::unique_ptr<ast::Statement> parseStatement();

  // 解析表达式语句
  std::unique_ptr<ast::ExprStmt> parseExprStmt();

  // 解析返回语句
  std::unique_ptr<ast::ReturnStmt> parseReturnStmt();

  // 解析复合语句
  std::unique_ptr<ast::CompoundStmt> parseCompoundStmt();

  // 解析条件语句
  std::unique_ptr<ast::IfStmt> parseIfStmt();

  // 解析匹配语句
  std::unique_ptr<ast::MatchStmt> parseMatchStmt();

  // 解析匹配分支
  std::unique_ptr<ast::MatchArm> parseMatchArm();

  // 解析模式
  std::unique_ptr<ast::Pattern> parsePattern();

  // 解析for语句
  std::unique_ptr<ast::ForStmt> parseForStmt();

  // 解析for初始化
  std::unique_ptr<ast::Node> parseForInit();

  // 解析while语句
  std::unique_ptr<ast::WhileStmt> parseWhileStmt();

  // 解析do-while语句
  std::unique_ptr<ast::DoWhileStmt> parseDoWhileStmt();

  // 解析try语句
  std::unique_ptr<ast::Statement> parseTryStmt();

  // 解析catch语句
  std::unique_ptr<ast::CatchStmt> parseCatchStmt();

  // 解析throw语句
  std::unique_ptr<ast::Statement> parseThrowStmt();

  // 解析defer语句
  std::unique_ptr<ast::Statement> parseDeferStmt();

  // 解析表达式
  std::unique_ptr<ast::Expression> parseExpression();

  // 解析赋值表达式
  std::unique_ptr<ast::Expression> parseAssignmentExpr();

  // 解析条件表达式
  std::unique_ptr<ast::Expression> parseConditionalExpr();

  // 解析逻辑或表达式
  std::unique_ptr<ast::Expression> parseLogicalOrExpr();

  // 解析逻辑与表达式
  std::unique_ptr<ast::Expression> parseLogicalAndExpr();

  // 解析包含或表达式
  std::unique_ptr<ast::Expression> parseInclusiveOrExpr();

  // 解析排他或表达式
  std::unique_ptr<ast::Expression> parseExclusiveOrExpr();

  // 解析与表达式
  std::unique_ptr<ast::Expression> parseAndExpr();

  // 解析相等表达式
  std::unique_ptr<ast::Expression> parseEqualityExpr();

  // 解析关系表达式
  std::unique_ptr<ast::Expression> parseRelationalExpr();

  // 解析移位表达式
  std::unique_ptr<ast::Expression> parseShiftExpr();

  // 解析加法表达式
  std::unique_ptr<ast::Expression> parseAdditiveExpr();

  // 解析乘法表达式
  std::unique_ptr<ast::Expression> parseMultiplicativeExpr();

  // 解析一元表达式
  std::unique_ptr<ast::Expression> parseUnaryExpr();

  // 解析主表达式
  std::unique_ptr<ast::Expression> parsePrimaryExpr();

  // 解析后缀表达式
  std::unique_ptr<ast::Expression>
  parsePostfixExpr(std::unique_ptr<ast::Expression> expr);

  // 解析函数调用
  std::unique_ptr<ast::Expression>
  parseCallExpr(std::unique_ptr<ast::Expression> callee);

  // 解析数组访问
  std::unique_ptr<ast::Expression>
  parseSubscriptExpr(std::unique_ptr<ast::Expression> object);

  // 解析成员访问
  std::unique_ptr<ast::Expression>
  parseMemberExpr(std::unique_ptr<ast::Expression> object);

  // 解析指针成员访问
  std::unique_ptr<ast::Expression>
  parsePointerMemberExpr(std::unique_ptr<ast::Expression> object);

  // 解析范围表达式
  std::unique_ptr<ast::Expression>
  parseRangeExpr(std::unique_ptr<ast::Expression> start);

  // 解析数组初始化
  std::unique_ptr<ast::Expression> parseArrayInit();

  // 解析Lambda表达式或数组初始化
  std::unique_ptr<ast::Expression> parseLambdaOrArrayInit();

  // 解析带捕获列表的Lambda表达式
  std::unique_ptr<ast::Expression>
  parseLambdaExprWithCaptures(std::vector<ast::Capture> captures);

  // 解析元组表达式或分组表达式
  std::unique_ptr<ast::Expression> parseTupleOrGrouping();

  // 解析元组表达式
  std::unique_ptr<ast::Expression> parseTupleExpr();

  // 解析结构体初始化
  std::unique_ptr<ast::Expression>
  parseStructInit(std::unique_ptr<ast::Type> type = nullptr);

  // 解析new表达式
  std::unique_ptr<ast::Expression> parseNewExpr();

  // 解析delete表达式
  std::unique_ptr<ast::Expression> parseDeleteExpr();

  // 解析Lambda表达式
  std::unique_ptr<ast::Expression> parseLambdaExpr();

  // 解析Lambda捕获列表
  std::vector<ast::Capture> parseLambdaCaptures();

  // 解析Lambda参数
  std::vector<std::unique_ptr<ast::Parameter>> parseLambdaParams();

  // 解析类型
  std::unique_ptr<ast::Type> parseType();

  // 解析类型后缀（指针、数组、切片、泛型）
  std::unique_ptr<ast::Type> parseTypeSuffix(std::unique_ptr<ast::Type> type);

  // 解析指针类型
  std::unique_ptr<ast::Type>
  parsePointerType(std::unique_ptr<ast::Type> baseType);

  // 解析数组类型
  std::unique_ptr<ast::Type>
  parseArrayType(std::unique_ptr<ast::Type> baseType);

  // 解析泛型类型
  std::unique_ptr<ast::Type> parseGenericType(const std::string &name);

  // 解析模板参数
  std::vector<std::unique_ptr<ast::TemplateParameter>>
  parseTemplateParameters();

  // 解析模板实参
  std::vector<std::unique_ptr<ast::Node>> parseTemplateArguments();

  // 解析参数列表
  std::vector<std::unique_ptr<ast::Node>> parseParameterList();

  // 解析参数
  std::unique_ptr<ast::Node> parseParameter();

  // 解析参数包
  std::unique_ptr<ast::Node> parseParameterPack();

  // 解析折叠表达式
  std::unique_ptr<ast::FoldExpr> parseFoldExpr();

  // 解析扩展操作符
  std::unique_ptr<ast::ExpansionExpr> parseExpansionExpr();

  // 解析编译期语句
  std::unique_ptr<ast::ComptimeStmt> parseComptimeStmt();

  // 解析where子句
  std::unique_ptr<ast::Node> parseWhereClause();

  // 解析requires子句
  std::unique_ptr<ast::Node> parseRequiresClause();

  // 解析requires表达式
  std::unique_ptr<ast::RequiresExpression> parseRequiresExpression();

  // 解析需求
  std::unique_ptr<ast::Requirement> parseRequirement();

  // 解析需求列表
  std::vector<std::unique_ptr<ast::Requirement>> parseRequirementList();

  // 检查当前token是否为类型开始
  bool isTypeStart() const;

  // 检查当前token是否为类型关键字
  bool isTypeKeyword() const;

  // 保存和恢复解析状态（用于尝试解析）
  struct ParserState {
    std::optional<lexer::Token> currentToken;
    std::optional<lexer::Token> previousToken;
    lexer::Lexer::LexerState lexerState;
  };

  ParserState saveState();
  void restoreState(const ParserState &state);
};

} // namespace parser
} // namespace c_hat
