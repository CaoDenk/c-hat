#pragma once

#include "../ast/AstNodes.h"
#include "../semantic/SymbolTable.h"
#include "LLVMIRGenerator.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace c_hat {
namespace llvm_codegen {

class LLVMCodeGenerator {
public:
  explicit LLVMCodeGenerator(const std::string &moduleName);
  ~LLVMCodeGenerator() = default;

  llvm::Module *getModule() { return generator_.getModule(); }
  llvm::IRBuilder<> *getBuilder() { return generator_.getBuilder(); }

  void generate(std::unique_ptr<ast::Program> program);

  bool verifyIR() { return generator_.verifyIR(); }
  void printIR() { generator_.printIR(); }
  bool writeIRToFile(const std::string &filename) {
    return generator_.writeIRToFile(filename);
  }

  // 生成目标文件
  bool emitObjectFile(const std::string &filename) {
    return generator_.emitObjectFile(filename);
  }

  // 生成汇编文件
  bool emitAssemblyFile(const std::string &filename) {
    return generator_.emitAssemblyFile(filename);
  }

  // JIT 执行
  bool hasJIT() const { return generator_.hasJIT(); }
  int runJIT(const std::string &entryPoint = "main") {
    return generator_.runJIT(entryPoint);
  }

  // 添加外部函数符号
  void addExternalSymbol(const std::string &name, void *address) {
    generator_.addExternalSymbol(name, address);
  }

  // 循环优化
  void optimizeLoops();
  bool isLoopInvariant(llvm::Instruction *inst, llvm::BasicBlock *loopHeader);

  // 函数内联优化
  void optimizeFunctionInlining();

private:
  LLVMIRGenerator generator_;

  bool currentFunctionHasTry_ = false;
  llvm::GlobalVariable *currentJmpBuf_ = nullptr;
  llvm::StructType *jmpBufType_ = nullptr;
  std::vector<std::unique_ptr<ast::CatchStmt>> catchStmts_;

  LLVMIRGenerator &generator() { return generator_; }
  llvm::LLVMContext &context() { return *generator_.getContext(); }
  llvm::Module *module() { return generator_.getModule(); }
  llvm::IRBuilder<> *builder() { return generator_.getBuilder(); }

  llvm::Value *generateDeclaration(std::unique_ptr<ast::Declaration> decl);

  llvm::Value *generateVariableDecl(std::unique_ptr<ast::VariableDecl> varDecl);
  llvm::Value *generateTupleDestructuringDecl(
      std::unique_ptr<ast::TupleDestructuringDecl> tupleDecl);
  llvm::Value *
  generateFunctionDecl(std::unique_ptr<ast::FunctionDecl> funcDecl);
  void createFunctionPrototype(ast::FunctionDecl *funcDecl);
  llvm::Value *
  generateFunctionBody(std::unique_ptr<ast::FunctionDecl> funcDecl);
  llvm::Value *generateClassDecl(std::unique_ptr<ast::ClassDecl> classDecl);
  llvm::Value *generateStructDecl(std::unique_ptr<ast::StructDecl> structDecl);
  llvm::Value *
  generateClassMemberFunction(std::unique_ptr<ast::FunctionDecl> funcDecl,
                              const std::string &className, bool isConstructor,
                              bool isDestructor);
  llvm::Value *
  generateExtensionMemberFunction(std::unique_ptr<ast::FunctionDecl> funcDecl,
                                  const std::string &structName);
  llvm::Value *
  generateTypeAliasDecl(std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl);
  llvm::Value *generateGetterDecl(std::unique_ptr<ast::GetterDecl> getterDecl);
  llvm::Value *generateSetterDecl(std::unique_ptr<ast::SetterDecl> setterDecl);
  llvm::Value *
  generateExtensionDecl(std::unique_ptr<ast::ExtensionDecl> extensionDecl);

  llvm::Value *generateExternDecl(std::unique_ptr<ast::ExternDecl> externDecl);

  llvm::Value *
  generateNamespaceDecl(std::unique_ptr<ast::NamespaceDecl> namespaceDecl);

  llvm::Value *generateStatement(std::unique_ptr<ast::Statement> stmt);
  llvm::Value *generateExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt);
  llvm::Value *
  generateCompoundStmt(std::unique_ptr<ast::CompoundStmt> compoundStmt);
  llvm::Value *generateReturnStmt(std::unique_ptr<ast::ReturnStmt> returnStmt);
  llvm::Value *generateIfStmt(std::unique_ptr<ast::IfStmt> ifStmt);
  llvm::Value *generateWhileStmt(std::unique_ptr<ast::WhileStmt> whileStmt);
  llvm::Value *generateForStmt(std::unique_ptr<ast::ForStmt> forStmt);
  llvm::Value *generateBreakStmt(std::unique_ptr<ast::BreakStmt> breakStmt);
  llvm::Value *
  generateContinueStmt(std::unique_ptr<ast::ContinueStmt> continueStmt);
  llvm::Value *
  generateMatchStmt(std::unique_ptr<ast::MatchStmt> matchStmt); // 异常处理
  llvm::Value *generateTryStmt(std::unique_ptr<ast::TryStmt> tryStmt);
  llvm::Value *generateThrowStmt(std::unique_ptr<ast::ThrowStmt> throwStmt);
  void checkExceptionAfterCall();

  // 其他语句
  llvm::Value *generateDeferStmt(std::unique_ptr<ast::DeferStmt> deferStmt);

  // 协程相关
  llvm::Value *generateYieldStmt(std::unique_ptr<ast::YieldStmt> yieldStmt);
  llvm::Value *generateAwaitExpr(std::unique_ptr<ast::UnaryExpr> awaitExpr);

  // Goto 和 Label
  llvm::Value *generateGotoStmt(std::unique_ptr<ast::GotoStmt> gotoStmt);
  llvm::Value *generateLabelStmt(std::unique_ptr<ast::LabelStmt> labelStmt);
  llvm::Value *generateExpression(std::unique_ptr<ast::Expression> expr);
  llvm::Value *getExpressionLValue(std::unique_ptr<ast::Expression> expr);
  llvm::Value *generateBinaryExpr(std::unique_ptr<ast::BinaryExpr> binaryExpr);
  llvm::Value *generateUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr,
                                 bool isLValue = false);
  llvm::Value *generateCallExpr(std::unique_ptr<ast::CallExpr> callExpr);
  llvm::Value *generateMemberExpr(std::unique_ptr<ast::MemberExpr> memberExpr);
  llvm::Value *
  generateSubscriptExpr(std::unique_ptr<ast::SubscriptExpr> subscriptExpr,
                        bool isLValue = false);
  llvm::Value *generateThisExpr(std::unique_ptr<ast::ThisExpr> thisExpr);
  llvm::Value *generateSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr);
  llvm::Value *generateSuperExpr(std::unique_ptr<ast::SuperExpr> superExpr);
  llvm::Value *generateNewExpr(std::unique_ptr<ast::NewExpr> newExpr);
  llvm::Value *generateDeleteExpr(std::unique_ptr<ast::DeleteExpr> deleteExpr);
  llvm::Value *generateFoldExpr(std::unique_ptr<ast::FoldExpr> foldExpr);
  llvm::Value *
  generateExpansionExpr(std::unique_ptr<ast::ExpansionExpr> expansionExpr);
  llvm::Value *
  generateArrayInitExpr(std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr);
  llvm::Value *
  generateStructInitExpr(std::unique_ptr<ast::StructInitExpr> structInitExpr);
  llvm::Value *generateTupleExpr(std::unique_ptr<ast::TupleExpr> tupleExpr);
  llvm::Value *generateLambdaExpr(std::unique_ptr<ast::LambdaExpr> lambdaExpr);
  llvm::Value *
  generateReflectionExpr(std::unique_ptr<ast::ReflectionExpr> reflectionExpr);
  llvm::Value *generateLiteral(std::unique_ptr<ast::Literal> literal);
  llvm::Value *generateIdentifier(std::unique_ptr<ast::Identifier> ident);

  llvm::Type *generateType(ast::Type *type);
  llvm::Value *generateTypeExpr(std::unique_ptr<ast::Type> type);

  // 类型辅助函数
  bool isSliceType(ast::Type *type);
  bool isArrayType(ast::Type *type);
  bool isPointerType(ast::Type *type);
  bool isReferenceType(ast::Type *type);
  bool isFunctionType(ast::Type *type);
  bool isClassType(ast::Type *type);
  bool isStructType(ast::Type *type);
  bool isEnumType(ast::Type *type);
  bool isTupleType(ast::Type *type);
  bool isGenericType(ast::Type *type);
  bool isReadonlyType(ast::Type *type);

  // 辅助函数
  llvm::Type *getLiteralViewType();
  std::string getTypeName(ast::Type *type);
  std::string getLLVMTypeName(llvm::Type *type);
  std::string mangleFunctionName(const std::string &funcName,
                                 const std::vector<ast::Type *> &paramTypes);

  // 状态管理
  llvm::Function *currentFunction_ = nullptr;
  llvm::StructType *literalViewType_ = nullptr;
  std::unordered_map<std::string, llvm::Value *> namedValues_;
  std::vector<std::unique_ptr<ast::Expression>> deferExpressions_;
  std::vector<std::string> namespaceStack_;
  std::unordered_map<ast::FunctionDecl *, std::vector<std::string>>
      functionNamespaces_;
  std::vector<std::unique_ptr<ast::Declaration>> *funcDecls_ = nullptr;

  // 类型映射
  std::unordered_map<std::string, llvm::StructType *> structTypes_;
  std::unordered_map<std::string, llvm::StructType *> sliceTypes_;
  std::unordered_map<std::string, std::unordered_map<std::string, unsigned>>
      structInfo_;
  std::unordered_map<std::string, llvm::Function *> functions_;

  // 类型缓存，用于提高代码生成效率
  std::unordered_map<ast::Type *, llvm::Type *> typeCache_;

  // 错误处理
  int errorCount_ = 0;
  bool hasErrors_ = false;

  // 错误处理方法
  void error(const std::string &message, ast::Node *node = nullptr);
  void warning(const std::string &message, ast::Node *node = nullptr);
  bool hasErrors() const { return hasErrors_; }
  int getErrorCount() const { return errorCount_; }

  // 延迟变量
  struct LateVariableInfo {
    bool isInitialized;
    ast::VariableDecl *decl;
  };
  std::unordered_map<std::string, LateVariableInfo> lateVariables_;

  // 协程状态
  bool currentFunctionIsCoroutine_ = false;
  int coroutineStateIndex_ = 0;
  llvm::AllocaInst *coroutineStateAlloca_ = nullptr;
  llvm::AllocaInst *coroutinePromiseAlloca_ = nullptr;
  std::unordered_map<std::string, llvm::BasicBlock *> labelBlocks_;
  std::unordered_map<std::string, llvm::AllocaInst *> coroutineLocals_;
};

} // namespace llvm_codegen
} // namespace c_hat
