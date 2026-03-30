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
  SemanticAnalyzer(const std::string &stdlibPath = "",
                   bool requireMainFunction = true);

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

  // 当前分析的程序
  ast::Program *currentProgram_ = nullptr;

  // 隐式转换运算符：源类型名称 -> 目标类型名称 -> 函数符号
  std::unordered_map<
      std::string,
      std::unordered_map<std::string, std::shared_ptr<FunctionSymbol>>>
      implicitOperators_;

  // 作用域类型栈，用于跟踪当前是否在循环或switch语句中
  std::vector<std::string> scopeStack;

  // 当前循环嵌套深度
  int loopDepth_ = 0;

  // 分析声明
  void analyzeDeclaration(ast::Declaration *declaration);

  // 分析变量声明
  void analyzeVariableDecl(ast::VariableDecl *varDecl);

  // 分析元组解构声明
  void analyzeTupleDestructuringDecl(ast::TupleDestructuringDecl *decl);

  // 分析函数声明
  void analyzeFunctionDecl(
      ast::FunctionDecl *funcDecl,
      std::shared_ptr<types::ClassType> currentClassType = nullptr,
      bool analyzeBody = true);

  // 分析类声明
  void analyzeClassDecl(ast::ClassDecl *classDecl);

  // 分析接口声明
  void analyzeInterfaceDecl(ast::InterfaceDecl *interfaceDecl);

  // 分析结构体声明
  void analyzeStructDecl(ast::StructDecl *structDecl);

  // 分析枚举声明
  void analyzeEnumDecl(ast::EnumDecl *enumDecl);

  // 分析类型别名声明
  void analyzeTypeAliasDecl(ast::TypeAliasDecl *typeAliasDecl);

  // 分析模块声明
  void analyzeModuleDecl(ast::ModuleDecl *moduleDecl);

  // 分析导入声明
  void analyzeImportDecl(ast::ImportDecl *importDecl);

  // 分析扩展声明
  void analyzeExtensionDecl(ast::ExtensionDecl *extensionDecl);

  // 分析getter声明
  void analyzeGetterDecl(ast::GetterDecl *getterDecl);

  // 分析setter声明
  void analyzeSetterDecl(ast::SetterDecl *setterDecl);

  // 分析外部声明块
  void analyzeExternDecl(ast::ExternDecl *externDecl);

  // 分析命名空间声明
  void analyzeNamespaceDecl(ast::NamespaceDecl *namespaceDecl);

  // 分析语句
  std::shared_ptr<types::Type> analyzeStatement(
      ast::Statement *stmt,
      std::shared_ptr<types::ClassType> currentClassType = nullptr);

  // 分析表达式语句
  std::shared_ptr<types::Type> analyzeExprStmt(ast::ExprStmt *exprStmt);

  // 分析复合语句
  std::shared_ptr<types::Type> analyzeCompoundStmt(
      ast::CompoundStmt *compoundStmt,
      std::shared_ptr<types::ClassType> currentClassType = nullptr);

  // 分析返回语句
  std::shared_ptr<types::Type> analyzeReturnStmt(ast::ReturnStmt *returnStmt);

  // 分析if语句
  std::shared_ptr<types::Type> analyzeIfStmt(ast::IfStmt *ifStmt);

  // 分析while语句
  std::shared_ptr<types::Type> analyzeWhileStmt(ast::WhileStmt *whileStmt);

  // 分析for语句
  std::shared_ptr<types::Type> analyzeForStmt(ast::ForStmt *forStmt);

  // 分析break语句
  std::shared_ptr<types::Type> analyzeBreakStmt(ast::BreakStmt *breakStmt);

  // 分析continue语句
  std::shared_ptr<types::Type>
  analyzeContinueStmt(ast::ContinueStmt *continueStmt);

  // 分析match语句
  std::shared_ptr<types::Type> analyzeMatchStmt(ast::MatchStmt *matchStmt);

  // 分析try语句
  std::shared_ptr<types::Type> analyzeTryStmt(ast::TryStmt *tryStmt);

  // 分析throw语句
  std::shared_ptr<types::Type> analyzeThrowStmt(ast::ThrowStmt *throwStmt);

  // 分析defer语句
  std::shared_ptr<types::Type> analyzeDeferStmt(ast::DeferStmt *deferStmt);

  // 分析表达式
  std::shared_ptr<types::Type> analyzeExpression(ast::Expression *expression);

  // 分析二元表达式
  std::shared_ptr<types::Type> analyzeBinaryExpr(ast::BinaryExpr *binaryExpr);

  // 分析一元表达式
  std::shared_ptr<types::Type> analyzeUnaryExpr(ast::UnaryExpr *unaryExpr);

  // 分析标识符表达式
  std::shared_ptr<types::Type>
  analyzeIdentifierExpr(ast::Identifier *identifier);

  // 分析字面量表达式
  std::shared_ptr<types::Type> analyzeLiteralExpr(ast::Literal *literal);

  // 分析函数调用表达式
  std::shared_ptr<types::Type> analyzeCallExpr(ast::CallExpr *callExpr);

  // 分析成员访问表达式
  std::shared_ptr<types::Type> analyzeMemberExpr(ast::MemberExpr *memberExpr);

  // 分析下标访问表达式
  std::shared_ptr<types::Type>
  analyzeSubscriptExpr(ast::SubscriptExpr *subscriptExpr);

  // 分析new表达式
  std::shared_ptr<types::Type> analyzeNewExpr(ast::NewExpr *newExpr);

  // 分析delete表达式
  std::shared_ptr<types::Type> analyzeDeleteExpr(ast::DeleteExpr *deleteExpr);

  // 分析this表达式
  std::shared_ptr<types::Type> analyzeThisExpr(ast::ThisExpr *thisExpr);

  // 分析super表达式
  std::shared_ptr<types::Type> analyzeSuperExpr(ast::SuperExpr *superExpr);

  // 分析self表达式
  std::shared_ptr<types::Type> analyzeSelfExpr(ast::SelfExpr *selfExpr);

  // 分析展开表达式
  std::shared_ptr<types::Type>
  analyzeExpansionExpr(ast::ExpansionExpr *expansionExpr);

  // 分析lambda表达式
  std::shared_ptr<types::Type> analyzeLambdaExpr(ast::LambdaExpr *lambdaExpr);

  // 分析数组初始化表达式
  std::shared_ptr<types::Type>
  analyzeArrayInitExpr(ast::ArrayInitExpr *arrayInitExpr);

  // 分析结构体初始化表达式
  std::shared_ptr<types::Type>
  analyzeStructInitExpr(ast::StructInitExpr *structInitExpr);

  // 分析元组表达式
  std::shared_ptr<types::Type> analyzeTupleExpr(ast::TupleExpr *tupleExpr);

  // 分析类型
  std::shared_ptr<types::Type> analyzeType(const ast::Type *type);

  // 根据类型名称分析类型
  std::shared_ptr<types::Type> analyzeTypeByName(const std::string &typeName);

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

  // 将 Visibility 转换为 AccessModifier
  types::AccessModifier visibilityToAccessModifier(Visibility visibility);

  // 报告错误
  void error(const std::string &message, const ast::Node &node);
  void error(const std::string &message);

  // 检查循环继承
  bool checkCircularInheritance(const types::ClassType *derived,
                                const types::ClassType *base);

  // 检查类是否实现了所有接口方法
  void checkInterfaceImplementation(ast::ClassDecl *classDecl,
                                    types::ClassType *classType);

  // 检查访问控制
  bool checkAccessControl(types::AccessModifier access,
                          const types::ClassType *ownerClass);

  // 检查一个类是否是另一个类的子类
  bool isSubclassOf(const types::ClassType *derived,
                    const types::ClassType *base);

  // 分析模板参数
  std::vector<std::shared_ptr<types::Type>> analyzeTemplateParameters(
      const std::vector<std::unique_ptr<ast::Node>> &params);

  // 分析模板实参
  std::vector<std::shared_ptr<types::Type>>
  analyzeTemplateArguments(const std::vector<std::unique_ptr<ast::Node>> &args);

  // 实例化模板类型
  std::shared_ptr<types::Type> instantiateTemplateType(
      const std::shared_ptr<types::Type> &templateType,
      const std::vector<std::shared_ptr<types::Type>> &typeArguments);

  // 是否有错误
  bool hasError_ = false;

  // 是否需要 main 函数
  bool requireMainFunction_ = true;

  // 当前函数的返回类型
  std::shared_ptr<types::Type> currentFunctionReturnType_;

  // 当前类名
  std::string currentClassName_;

  // 当前方法是否为 static（用于检查静态方法体内非法访问非静态成员）
  bool currentFuncIsStatic_ = false;

  // late 变量的初始化状态跟踪
  struct LateVariableStatus {
    bool isInitialized;
    const ast::VariableDecl *decl;
  };
  std::unordered_map<std::string, LateVariableStatus> lateVariables_;
  using LateVariablesSnapshot =
      std::unordered_map<std::string, LateVariableStatus>;

  LateVariablesSnapshot snapshotLateVariables() const;
  void restoreLateVariables(const LateVariablesSnapshot &snapshot);
  void mergeLateVariablesFromBranch(const LateVariablesSnapshot &before,
                                    const LateVariablesSnapshot &branch,
                                    bool useBranchState);
  bool statementDefinitelyReturns(const ast::Statement *stmt) const;

  // 初始化内置符号
  void initializeBuiltinSymbols();
};

} // namespace semantic
} // namespace c_hat
