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

private:
  LLVMIRGenerator generator_;

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
  llvm::Value *generateClassDecl(std::unique_ptr<ast::ClassDecl> classDecl);
  llvm::Value *generateStructDecl(std::unique_ptr<ast::StructDecl> structDecl);
  llvm::Value *
  generateClassMemberFunction(std::unique_ptr<ast::FunctionDecl> funcDecl,
                              const std::string &className, bool isConstructor,
                              bool isDestructor);
  llvm::Value *
  generateTypeAliasDecl(std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl);
  llvm::Value *generateGetterDecl(std::unique_ptr<ast::GetterDecl> getterDecl);
  llvm::Value *generateSetterDecl(std::unique_ptr<ast::SetterDecl> setterDecl);
  llvm::Value *
  generateExtensionDecl(std::unique_ptr<ast::ExtensionDecl> extensionDecl);

  llvm::Value *generateExternDecl(std::unique_ptr<ast::ExternDecl> externDecl);

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
  llvm::Value *generateMatchStmt(std::unique_ptr<ast::MatchStmt> matchStmt);
  llvm::Value *generateTryStmt(std::unique_ptr<ast::TryStmt> tryStmt);
  llvm::Value *generateThrowStmt(std::unique_ptr<ast::ThrowStmt> throwStmt);
  llvm::Value *generateDeferStmt(std::unique_ptr<ast::DeferStmt> deferStmt);

  llvm::Value *generateExpression(std::unique_ptr<ast::Expression> expr);
  llvm::Value *getExpressionLValue(std::unique_ptr<ast::Expression> expr);
  llvm::Value *generateBinaryExpr(std::unique_ptr<ast::BinaryExpr> binaryExpr);
  llvm::Value *generateUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr,
                                 bool isLValue = false);
  llvm::Value *
  generateIdentifierExpr(std::unique_ptr<ast::Identifier> identifier,
                         bool isLValue = false);
  llvm::Value *generateLiteralExpr(std::unique_ptr<ast::Literal> literal);
  llvm::Value *generateCallExpr(std::unique_ptr<ast::CallExpr> callExpr);
  llvm::Value *generateMemberExpr(std::unique_ptr<ast::MemberExpr> memberExpr,
                                  bool isLValue = false);
  llvm::Value *
  generateSubscriptExpr(std::unique_ptr<ast::SubscriptExpr> subscriptExpr);
  llvm::Value *generateNewExpr(std::unique_ptr<ast::NewExpr> newExpr);
  llvm::Value *generateDeleteExpr(std::unique_ptr<ast::DeleteExpr> deleteExpr);
  llvm::Value *generateThisExpr(std::unique_ptr<ast::ThisExpr> thisExpr);
  llvm::Value *generateSuperExpr(std::unique_ptr<ast::SuperExpr> superExpr);
  llvm::Value *generateSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr);
  llvm::Value *
  generateExpansionExpr(std::unique_ptr<ast::ExpansionExpr> expansionExpr);
  llvm::Value *generateLambdaExpr(std::unique_ptr<ast::LambdaExpr> lambdaExpr);
  llvm::Value *
  generateArrayInitExpr(std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr);
  llvm::Value *
  generateStructInitExpr(std::unique_ptr<ast::StructInitExpr> structInitExpr);
  llvm::Value *generateTupleExpr(std::unique_ptr<ast::TupleExpr> tupleExpr);

  llvm::Type *generateType(const ast::Type *type);

  llvm::Type *generatePrimitiveType(const ast::PrimitiveType *primitiveType);
  llvm::Type *getPrimitiveType(const std::string &typeName);
  llvm::Type *getPointerType(llvm::Type *baseType, bool nullable);
  llvm::Type *getArrayType(llvm::Type *elementType, int64_t size);
  llvm::Type *getSliceType(llvm::Type *elementType);
  llvm::Type *getLiteralViewType();

  llvm::Value *createStringLiteral(const std::string &str);

  std::unordered_map<std::string, llvm::Value *> namedValues_;
  std::unordered_map<std::string, llvm::Function *> functions_;
  std::unordered_map<std::string, llvm::StructType *> structTypes_;
  std::unordered_map<std::string, std::unordered_map<std::string, unsigned>>
      structInfo_;
  llvm::Function *currentFunction_ = nullptr;
  llvm::BasicBlock *continueBlock_ = nullptr;
  llvm::BasicBlock *breakBlock_ = nullptr;

  // defer 语句收集
  std::vector<std::unique_ptr<ast::Expression>> deferExpressions_;
};

} // namespace llvm_codegen
} // namespace c_hat
