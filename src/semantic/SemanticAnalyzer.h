#pragma once

#include "../ast/AstNodes.h"
#include "../types/Type.h"
#include "ModuleLoader.h"
#include "SymbolTable.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace c_hat {
namespace semantic {

class SemanticAnalyzer {
public:
  SemanticAnalyzer(const std::string &stdlibPath = "");

  // 分析整个程序
  void analyze(ast::Program &program);

  // 获取符号表
  SymbolTable &getSymbolTable() { return symbolTable; }

  // 检查是否有错误
  bool hasError() const { return hasError_; }

private:
  // 符号表
  SymbolTable symbolTable;

  // 模块加载器
  std::unique_ptr<ModuleLoader> moduleLoader_;

  // 隐式转换运算符：源类型名称 -> 目标类型名称 -> 函数符号
  std::unordered_map<
      std::string,
      std::unordered_map<std::string, std::shared_ptr<FunctionSymbol>>>
      implicitOperators_;

  // 作用域类型栈，用于跟踪当前是否在循环或switch语句中
  std::vector<std::string> scopeStack;

  // 分析声明
  void analyzeDeclaration(std::unique_ptr<ast::Declaration> declaration);

  // 分析变量声明
  void analyzeVariableDecl(std::unique_ptr<ast::VariableDecl> varDecl);

  // 分析元组解构声明
  void analyzeTupleDestructuringDecl(
      std::unique_ptr<ast::TupleDestructuringDecl> decl);

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

  // 分析getter声明
  void analyzeGetterDecl(std::unique_ptr<ast::GetterDecl> getterDecl);

  // 分析setter声明
  void analyzeSetterDecl(std::unique_ptr<ast::SetterDecl> setterDecl);

  // 分析外部声明块
  void analyzeExternDecl(std::unique_ptr<ast::ExternDecl> externDecl);

  // 分析语句
  std::shared_ptr<types::Type>
  analyzeStatement(std::unique_ptr<ast::Statement> stmt);

  // 分析表达式语句
  std::shared_ptr<types::Type>
  analyzeExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt);

  // 分析复合语句
  std::shared_ptr<types::Type>
  analyzeCompoundStmt(std::unique_ptr<ast::CompoundStmt> compoundStmt);

  // 分析返回语句
  std::shared_ptr<types::Type>
  analyzeReturnStmt(std::unique_ptr<ast::ReturnStmt> returnStmt);

  // 分析if语句
  std::shared_ptr<types::Type>
  analyzeIfStmt(std::unique_ptr<ast::IfStmt> ifStmt);

  // 分析while语句
  std::shared_ptr<types::Type>
  analyzeWhileStmt(std::unique_ptr<ast::WhileStmt> whileStmt);

  // 分析for语句
  std::shared_ptr<types::Type>
  analyzeForStmt(std::unique_ptr<ast::ForStmt> forStmt);

  // 分析break语句
  std::shared_ptr<types::Type>
  analyzeBreakStmt(std::unique_ptr<ast::BreakStmt> breakStmt);

  // 分析continue语句
  std::shared_ptr<types::Type>
  analyzeContinueStmt(std::unique_ptr<ast::ContinueStmt> continueStmt);

  // 分析match语句
  std::shared_ptr<types::Type>
  analyzeMatchStmt(std::unique_ptr<ast::MatchStmt> matchStmt);

  // 分析try语句
  std::shared_ptr<types::Type>
  analyzeTryStmt(std::unique_ptr<ast::TryStmt> tryStmt);

  // 分析throw语句
  std::shared_ptr<types::Type>
  analyzeThrowStmt(std::unique_ptr<ast::ThrowStmt> throwStmt);

  // 分析defer语句
  std::shared_ptr<types::Type>
  analyzeDeferStmt(std::unique_ptr<ast::DeferStmt> deferStmt);

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

  // 分析super表达式
  std::shared_ptr<types::Type>
  analyzeSuperExpr(std::unique_ptr<ast::SuperExpr> superExpr);

  // 分析self表达式
  std::shared_ptr<types::Type>
  analyzeSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr);

  // 分析展开表达式
  std::shared_ptr<types::Type>
  analyzeExpansionExpr(std::unique_ptr<ast::ExpansionExpr> expansionExpr);

  // 分析lambda表达式
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

  // 分析基本类型
  std::shared_ptr<types::Type>
  analyzePrimitiveType(const ast::PrimitiveType *primitiveType);

  // 分析指针类型
  std::shared_ptr<types::Type>
  analyzePointerType(const ast::PointerType *pointerType);

  // 分析数组类型
  std::shared_ptr<types::Type>
  analyzeArrayType(const ast::ArrayType *arrayType);

  // 分析切片类型
  std::shared_ptr<types::Type>
  analyzeSliceType(const ast::SliceType *sliceType);

  // 分析引用类型
  std::shared_ptr<types::Type>
  analyzeReferenceType(const ast::ReferenceType *referenceType);

  // 分析函数类型
  std::shared_ptr<types::Type>
  analyzeFunctionType(const ast::FunctionType *functionType);

  // 分析命名类型
  std::shared_ptr<types::Type>
  analyzeNamedType(const ast::NamedType *namedType);

  // 分析元组类型
  std::shared_ptr<types::Type>
  analyzeTupleType(const ast::TupleType *tupleType);

  // 检查表达式是否为左值
  bool isLValue(const ast::Expression &expr) const;

  // 检查类型是否兼容
  bool isTypeCompatible(const std::shared_ptr<types::Type> &expected,
                        const std::shared_ptr<types::Type> &actual);

  // 尝试进行隐式类型转换
  std::shared_ptr<types::Type>
  tryImplicitConversion(const std::shared_ptr<types::Type> &expected,
                        const std::shared_ptr<types::Type> &actual);

  // 解析可见性修饰符
  Visibility parseVisibility(const std::vector<std::string> &specifiers);

  // 报告错误
  void error(const std::string &message, const ast::Node &node);

  // 是否有错误
  bool hasError_ = false;

  // 当前函数的返回类型
  std::shared_ptr<types::Type> currentFunctionReturnType_;

  // 当前类名
  std::string currentClassName_;

  // 初始化内置符号
  void initializeBuiltinSymbols();
};

} // namespace semantic
} // namespace c_hat
