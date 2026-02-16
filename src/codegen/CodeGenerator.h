#pragma once

#include "../ast/AstNodes.h"
#include "../types/Types.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace codegen {

class CodeGenerator {
public:
  CodeGenerator();
  ~CodeGenerator() = default;

  // 生成代码
  std::string generate(std::unique_ptr<ast::Program> program);

private:
  // 生成声明代码
  std::string
  generateDeclaration(std::unique_ptr<ast::Declaration> declaration);

  // 生成变量声明代码
  std::string generateVariableDecl(std::unique_ptr<ast::VariableDecl> varDecl);

  // 生成函数声明代码
  std::string generateFunctionDecl(std::unique_ptr<ast::FunctionDecl> funcDecl);

  // 生成类声明代码
  std::string generateClassDecl(std::unique_ptr<ast::ClassDecl> classDecl);

  // 生成类型别名声明代码
  std::string
  generateTypeAliasDecl(std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl);

  // 生成语句代码
  std::string generateStatement(std::unique_ptr<ast::Statement> statement);

  // 生成表达式语句代码
  std::string generateExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt);

  // 生成复合语句代码
  std::string
  generateCompoundStmt(std::unique_ptr<ast::CompoundStmt> compoundStmt);

  // 生成返回语句代码
  std::string generateReturnStmt(std::unique_ptr<ast::ReturnStmt> returnStmt);

  // 生成comptime语句代码
  std::string
  generateComptimeStmt(std::unique_ptr<ast::ComptimeStmt> comptimeStmt);

  // 生成表达式代码
  std::string generateExpression(std::unique_ptr<ast::Expression> expression);

  // 生成二元表达式代码
  std::string generateBinaryExpr(std::unique_ptr<ast::BinaryExpr> binaryExpr);

  // 生成一元表达式代码
  std::string generateUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr);

  // 生成标识符表达式代码
  std::string
  generateIdentifierExpr(std::unique_ptr<ast::Identifier> identifier);

  // 生成字面量表达式代码
  std::string generateLiteralExpr(std::unique_ptr<ast::Literal> literal);

  // 生成函数调用表达式代码
  std::string generateCallExpr(std::unique_ptr<ast::CallExpr> callExpr);

  // 生成成员访问表达式代码
  std::string generateMemberExpr(std::unique_ptr<ast::MemberExpr> memberExpr);

  // 生成下标访问表达式代码
  std::string
  generateSubscriptExpr(std::unique_ptr<ast::SubscriptExpr> subscriptExpr);

  // 生成new表达式代码
  std::string generateNewExpr(std::unique_ptr<ast::NewExpr> newExpr);

  // 生成super表达式代码
  std::string generateSuperExpr(std::unique_ptr<ast::SuperExpr> superExpr);

  // 生成可变参数展开表达式代码
  std::string
  generateExpansionExpr(std::unique_ptr<ast::ExpansionExpr> expansionExpr);

  // 生成lambda表达式代码
  std::string generateLambdaExpr(std::unique_ptr<ast::LambdaExpr> lambdaExpr);

  // 生成元组表达式代码
  std::string generateTupleExpr(std::unique_ptr<ast::TupleExpr> tupleExpr);

  // 生成类型代码
  std::string generateType(std::unique_ptr<ast::Type> type);

  // 缩进级别
  int indentLevel;

  // 生成缩进
  std::string indent();
};

} // namespace codegen
} // namespace c_hat
