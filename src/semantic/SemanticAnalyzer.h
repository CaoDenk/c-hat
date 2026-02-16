
#pragma once

#include "../ast/AstNodes.h"
#include "ModuleLoader.h"
#include "SymbolTable.h"
#include <memory>
#include <string>

namespace c_hat {
namespace semantic {

// 语义分析器类
class SemanticAnalyzer {
public:
  SemanticAnalyzer(const std::string &stdlibPath = "");

  // 分析整个程序
  void analyze(std::unique_ptr<ast::Program> program);

  // 获取符号表
  SymbolTable &getSymbolTable() { return symbolTable; }

private:
  // 符号表
  SymbolTable symbolTable;

  // 模块加载器
  std::unique_ptr<ModuleLoader> moduleLoader_;

  // 作用域类型栈，用于跟踪当前是否在循环或switch语句中
  std::vector<std::string> scopeStack;

  // 分析声明
  void analyzeDeclaration(std::unique_ptr<ast::Declaration> declaration);

  // 分析变量声明
  void analyzeVariableDecl(std::unique_ptr<ast::VariableDecl> varDecl);

  // 分析函数声明
  void analyzeFunctionDecl(std::unique_ptr<ast::FunctionDecl> funcDecl);

  // 分析类声明
  void analyzeClassDecl(std::unique_ptr<ast::ClassDecl> classDecl);

  // 分析结构体声明
  void analyzeStructDecl(std::unique_ptr<ast::StructDecl> structDecl);

  // 分析枚举声明
  void analyzeEnumDecl(std::unique_ptr<ast::EnumDecl> enumDecl);

  // 分析类型别名声明
  void analyzeTypeAliasDecl(std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl);

  // 分析模块声明
  void analyzeModuleDecl(std::unique_ptr<ast::ModuleDecl> moduleDecl);

  // 分析导入声明
  void analyzeImportDecl(std::unique_ptr<ast::ImportDecl> importDecl);

  // 分析扩展声明
  void analyzeExtensionDecl(std::unique_ptr<ast::ExtensionDecl> extensionDecl);

  // 分析 Getter 声明
  void analyzeGetterDecl(std::unique_ptr<ast::GetterDecl> getterDecl);

  // 分析 Setter 声明
  void analyzeSetterDecl(std::unique_ptr<ast::SetterDecl> setterDecl);

  // 分析外部声明块
  void analyzeExternDecl(std::unique_ptr<ast::ExternDecl> externDecl);

  // 分析语句
  void analyzeStatement(std::unique_ptr<ast::Statement> statement);

  // 分析表达式语句
  void analyzeExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt);

  // 分析复合语句
  void analyzeCompoundStmt(std::unique_ptr<ast::CompoundStmt> compoundStmt);

  // 分析返回语句
  void analyzeReturnStmt(std::unique_ptr<ast::ReturnStmt> returnStmt);

  // 分析if语句
  void analyzeIfStmt(std::unique_ptr<ast::IfStmt> ifStmt);

  // 分析match语句
  void analyzeMatchStmt(std::unique_ptr<ast::MatchStmt> matchStmt);

  // 分析for语句
  void analyzeForStmt(std::unique_ptr<ast::ForStmt> forStmt);

  // 分析while语句
  void analyzeWhileStmt(std::unique_ptr<ast::WhileStmt> whileStmt);

  // 分析do-while语句
  void analyzeDoWhileStmt(std::unique_ptr<ast::DoWhileStmt> doWhileStmt);

  // 分析break语句
  void analyzeBreakStmt(std::unique_ptr<ast::BreakStmt> breakStmt);

  // 分析continue语句
  void analyzeContinueStmt(std::unique_ptr<ast::ContinueStmt> continueStmt);

  // 分析try语句
  void analyzeTryStmt(std::unique_ptr<ast::TryStmt> tryStmt);

  // 分析defer语句
  void analyzeDeferStmt(std::unique_ptr<ast::DeferStmt> deferStmt);

  // 分析comptime语句
  void analyzeComptimeStmt(std::unique_ptr<ast::ComptimeStmt> comptimeStmt);

  // 分析表达式
  std::shared_ptr<types::Type>
  analyzeExpression(std::unique_ptr<ast::Expression> expression);

  // 分析二元表达式
  std::shared_ptr<types::Type>
  analyzeBinaryExpr(std::unique_ptr<ast::BinaryExpr> binaryExpr);

  // 分析一元表达式
  std::shared_ptr<types::Type>
  analyzeUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr);

  // 分析标识符表达式
  std::shared_ptr<types::Type>
  analyzeIdentifierExpr(std::unique_ptr<ast::Identifier> identifier);

  // 分析字面量表达式
  std::shared_ptr<types::Type>
  analyzeLiteralExpr(std::unique_ptr<ast::Literal> literal);

  // 分析函数调用表达式
  std::shared_ptr<types::Type>
  analyzeCallExpr(std::unique_ptr<ast::CallExpr> callExpr);

  // 分析成员访问表达式
  std::shared_ptr<types::Type>
  analyzeMemberExpr(std::unique_ptr<ast::MemberExpr> memberExpr);

  // 分析下标访问表达式
  std::shared_ptr<types::Type>
  analyzeSubscriptExpr(std::unique_ptr<ast::SubscriptExpr> subscriptExpr);

  // 分析new表达式
  std::shared_ptr<types::Type>
  analyzeNewExpr(std::unique_ptr<ast::NewExpr> newExpr);

  // 分析delete表达式
  std::shared_ptr<types::Type>
  analyzeDeleteExpr(std::unique_ptr<ast::DeleteExpr> deleteExpr);

  // 分析this表达式
  std::shared_ptr<types::Type>
  analyzeThisExpr(std::unique_ptr<ast::ThisExpr> thisExpr);

  // 分析self表达式
  std::shared_ptr<types::Type>
  analyzeSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr);

  // 分析super表达式
  std::shared_ptr<types::Type>
  analyzeSuperExpr(std::unique_ptr<ast::SuperExpr> superExpr);

  // 分析可变参数展开表达式
  std::shared_ptr<types::Type>
  analyzeExpansionExpr(std::unique_ptr<ast::ExpansionExpr> expansionExpr);

  // 分析Lambda表达式
  std::shared_ptr<types::Type>
  analyzeLambdaExpr(std::unique_ptr<ast::LambdaExpr> lambdaExpr);

  // 分析数组初始化表达式
  std::shared_ptr<types::Type>
  analyzeArrayInitExpr(std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr);

  // 分析结构体初始化表达式
  std::shared_ptr<types::Type>
  analyzeStructInitExpr(std::unique_ptr<ast::StructInitExpr> structInitExpr);

  // 分析元组表达式
  std::shared_ptr<types::Type>
  analyzeTupleExpr(std::unique_ptr<ast::TupleExpr> tupleExpr);

  // 分析类型
  std::shared_ptr<types::Type> analyzeType(const ast::Type *type);

  // 检查类型兼容性
  bool checkTypeCompatibility(const types::Type &type1,
                              const types::Type &type2);

  // 报告错误
  void error(const std::string &message, const ast::Node &node);
};

} // namespace semantic
} // namespace c_hat
