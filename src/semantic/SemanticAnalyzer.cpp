#include "SemanticAnalyzer.h"
#include "../ast/statements/BreakStmt.h"
#include "../ast/statements/ContinueStmt.h"
#include "../lexer/Lexer.h"
#include "EnumSymbol.h"
#include "ModuleSymbol.h"
#include "StructSymbol.h"
#include <format>
#include <iostream>

namespace c_hat {
namespace semantic {

// 辅助函数：将 specifiers 字符串转换为 Visibility 枚举
static Visibility parseVisibility(const std::string &specifiers) {
  if (specifiers == "public") {
    return Visibility::Public;
  } else if (specifiers == "private") {
    return Visibility::Private;
  } else if (specifiers == "protected") {
    return Visibility::Protected;
  } else if (specifiers == "internal") {
    return Visibility::Internal;
  }
  return Visibility::Default;
}

// SemanticAnalyzer 构造函数
SemanticAnalyzer::SemanticAnalyzer(const std::string &stdlibPath)
    : hasError_(false) {
  if (!stdlibPath.empty()) {
    moduleLoader_ = std::make_unique<ModuleLoader>(stdlibPath);
  } else {
    moduleLoader_ = std::make_unique<ModuleLoader>("stdlib");
  }
  // 初始化符号表，添加内置类型
  // 添加所有原始类型作为类型别名符号
  struct BuiltinType {
    std::string name;
    types::PrimitiveType::Kind kind;
  };

  std::vector<BuiltinType> builtins = {
      {"void", types::PrimitiveType::Kind::Void},
      {"bool", types::PrimitiveType::Kind::Bool},
      {"byte", types::PrimitiveType::Kind::Byte},
      {"sbyte", types::PrimitiveType::Kind::SByte},
      {"short", types::PrimitiveType::Kind::Short},
      {"ushort", types::PrimitiveType::Kind::UShort},
      {"int", types::PrimitiveType::Kind::Int},
      {"uint", types::PrimitiveType::Kind::UInt},
      {"long", types::PrimitiveType::Kind::Long},
      {"ulong", types::PrimitiveType::Kind::ULong},
      {"float", types::PrimitiveType::Kind::Float},
      {"double", types::PrimitiveType::Kind::Double},
      {"fp16", types::PrimitiveType::Kind::Fp16},
      {"bf16", types::PrimitiveType::Kind::Bf16},
      {"char", types::PrimitiveType::Kind::Char}};

  for (const auto &builtin : builtins) {
    auto type = types::TypeFactory::getPrimitiveType(builtin.kind);
    auto symbol = std::make_shared<TypeAliasSymbol>(builtin.name, type);
    symbolTable.addSymbol(symbol);
  }

  // 添加 LiteralView 类型
  auto literalViewType = types::TypeFactory::getLiteralViewType();
  auto literalViewSymbol =
      std::make_shared<TypeAliasSymbol>("LiteralView", literalViewType);
  symbolTable.addSymbol(literalViewSymbol);
}

// 分析整个程序
void SemanticAnalyzer::analyze(ast::Program &program) {
  // 分析每个声明
  for (auto &declaration : program.declarations) {
    // 注意：这里我们不能用 std::move，因为后面代码生成阶段还需要这些
    // declaration 所以我们需要创建一个临时的 unique_ptr 来传递给
    // analyzeDeclaration 但是因为 analyzeDeclaration 是 void
    // 函数，所以我们可以用一个 dummy unique_ptr 调用它 不过因为所有 analyze*
    // 函数现在都是空实现，所以我们暂时不做任何实际工作
    // analyzeDeclaration(std::unique_ptr<ast::Declaration>(declaration.get()));
  }
}

// 分析声明
void SemanticAnalyzer::analyzeDeclaration(
    std::unique_ptr<ast::Declaration> declaration) {
  switch (declaration->getType()) {
  case ast::NodeType::VariableDecl:
    analyzeVariableDecl(std::unique_ptr<ast::VariableDecl>(
        static_cast<ast::VariableDecl *>(declaration.release())));
    break;
  case ast::NodeType::TupleDestructuringDecl:
    analyzeTupleDestructuringDecl(std::unique_ptr<ast::TupleDestructuringDecl>(
        static_cast<ast::TupleDestructuringDecl *>(declaration.release())));
    break;
  case ast::NodeType::FunctionDecl:
    analyzeFunctionDecl(std::unique_ptr<ast::FunctionDecl>(
        static_cast<ast::FunctionDecl *>(declaration.release())));
    break;
  case ast::NodeType::ClassDecl:
    analyzeClassDecl(std::unique_ptr<ast::ClassDecl>(
        static_cast<ast::ClassDecl *>(declaration.release())));
    break;
  case ast::NodeType::StructDecl:
    analyzeStructDecl(std::unique_ptr<ast::StructDecl>(
        static_cast<ast::StructDecl *>(declaration.release())));
    break;
  case ast::NodeType::EnumDecl:
    analyzeEnumDecl(std::unique_ptr<ast::EnumDecl>(
        static_cast<ast::EnumDecl *>(declaration.release())));
    break;
  case ast::NodeType::TypeAliasDecl:
    analyzeTypeAliasDecl(std::unique_ptr<ast::TypeAliasDecl>(
        static_cast<ast::TypeAliasDecl *>(declaration.release())));
    break;
  case ast::NodeType::ModuleDecl:
    analyzeModuleDecl(std::unique_ptr<ast::ModuleDecl>(
        static_cast<ast::ModuleDecl *>(declaration.release())));
    break;
  case ast::NodeType::ImportDecl:
    analyzeImportDecl(std::unique_ptr<ast::ImportDecl>(
        static_cast<ast::ImportDecl *>(declaration.release())));
    break;
  case ast::NodeType::ExtensionDecl:
    analyzeExtensionDecl(std::unique_ptr<ast::ExtensionDecl>(
        static_cast<ast::ExtensionDecl *>(declaration.release())));
    break;
  case ast::NodeType::GetterDecl:
    analyzeGetterDecl(std::unique_ptr<ast::GetterDecl>(
        static_cast<ast::GetterDecl *>(declaration.release())));
    break;
  case ast::NodeType::SetterDecl:
    analyzeSetterDecl(std::unique_ptr<ast::SetterDecl>(
        static_cast<ast::SetterDecl *>(declaration.release())));
    break;
  case ast::NodeType::ExternDecl:
    analyzeExternDecl(std::unique_ptr<ast::ExternDecl>(
        static_cast<ast::ExternDecl *>(declaration.release())));
    break;
  case ast::NodeType::NamespaceDecl:
    analyzeNamespaceDecl(std::unique_ptr<ast::NamespaceDecl>(
        static_cast<ast::NamespaceDecl *>(declaration.release())));
    break;
  default:
    // 未知的声明类型
    break;
  }
}

void SemanticAnalyzer::analyzeVariableDecl(
    std::unique_ptr<ast::VariableDecl> varDecl) {
  // 分析变量类型
  std::shared_ptr<types::Type> varType = nullptr;
  if (varDecl->type) {
    if (auto *typeNode = dynamic_cast<ast::Type *>(varDecl->type.get())) {
      varType = analyzeType(typeNode);
    }
  }

  // 检查 late 变量
  if (varDecl->isLate) {
    // late 变量不能有初始化器
    if (varDecl->initializer) {
      error("late variable cannot have initializer", *varDecl);
    }

    // 记录 late 变量的状态
    lateVariables_[varDecl->name] = {false, varDecl.get()};
  }

  // 添加到符号表
  auto varSymbol = std::make_shared<VariableSymbol>(
      varDecl->name, varType, ast::VariableKind::Explicit, varDecl->isConst);
  symbolTable.addSymbol(varSymbol);
}
void SemanticAnalyzer::analyzeTupleDestructuringDecl(
    std::unique_ptr<ast::TupleDestructuringDecl> decl) {}
void SemanticAnalyzer::analyzeFunctionDecl(
    std::unique_ptr<ast::FunctionDecl> funcDecl) {
  // 清空 late 变量表，因为它们的作用域是函数级别的
  lateVariables_.clear();

  // 分析函数体
  if (funcDecl->body) {
    if (auto *stmt = dynamic_cast<ast::Statement *>(funcDecl->body.get())) {
      analyzeStatement(std::unique_ptr<ast::Statement>(
          static_cast<ast::Statement *>(funcDecl->body.release())));
    }
  }
}
void SemanticAnalyzer::analyzeClassDecl(
    std::unique_ptr<ast::ClassDecl> classDecl) {}
void SemanticAnalyzer::analyzeStructDecl(
    std::unique_ptr<ast::StructDecl> structDecl) {}
void SemanticAnalyzer::analyzeEnumDecl(
    std::unique_ptr<ast::EnumDecl> enumDecl) {}
void SemanticAnalyzer::analyzeTypeAliasDecl(
    std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl) {}
void SemanticAnalyzer::analyzeModuleDecl(
    std::unique_ptr<ast::ModuleDecl> moduleDecl) {}
void SemanticAnalyzer::analyzeImportDecl(
    std::unique_ptr<ast::ImportDecl> importDecl) {}
void SemanticAnalyzer::analyzeExtensionDecl(
    std::unique_ptr<ast::ExtensionDecl> extensionDecl) {}
void SemanticAnalyzer::analyzeGetterDecl(
    std::unique_ptr<ast::GetterDecl> getterDecl) {}
void SemanticAnalyzer::analyzeSetterDecl(
    std::unique_ptr<ast::SetterDecl> setterDecl) {}
void SemanticAnalyzer::analyzeExternDecl(
    std::unique_ptr<ast::ExternDecl> externDecl) {}

void SemanticAnalyzer::analyzeNamespaceDecl(
    std::unique_ptr<ast::NamespaceDecl> namespaceDecl) {
  // 进入命名空间作用域
  scopeStack.push_back(namespaceDecl->name);

  // 分析命名空间中的成员
  for (auto &member : namespaceDecl->members) {
    if (auto *decl = dynamic_cast<ast::Declaration *>(member.get())) {
      analyzeDeclaration(std::unique_ptr<ast::Declaration>(decl));
    }
  }

  // 退出命名空间作用域
  scopeStack.pop_back();
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeStatement(std::unique_ptr<ast::Statement> stmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeCompoundStmt(
    std::unique_ptr<ast::CompoundStmt> compoundStmt) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeReturnStmt(
    std::unique_ptr<ast::ReturnStmt> returnStmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeIfStmt(std::unique_ptr<ast::IfStmt> ifStmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeWhileStmt(std::unique_ptr<ast::WhileStmt> whileStmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeForStmt(std::unique_ptr<ast::ForStmt> forStmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeBreakStmt(std::unique_ptr<ast::BreakStmt> breakStmt) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeContinueStmt(
    std::unique_ptr<ast::ContinueStmt> continueStmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeMatchStmt(std::unique_ptr<ast::MatchStmt> matchStmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTryStmt(std::unique_ptr<ast::TryStmt> tryStmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeThrowStmt(std::unique_ptr<ast::ThrowStmt> throwStmt) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeDeferStmt(std::unique_ptr<ast::DeferStmt> deferStmt) {
  return nullptr;
}

std::shared_ptr<types::Type> SemanticAnalyzer::analyzeExpression(
    std::unique_ptr<ast::Expression> expression) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeBinaryExpr(
    std::unique_ptr<ast::BinaryExpr> binaryExpr) {
  // 分析左操作数
  std::shared_ptr<types::Type> leftType =
      analyzeExpression(std::move(binaryExpr->left));

  // 分析右操作数
  std::shared_ptr<types::Type> rightType =
      analyzeExpression(std::move(binaryExpr->right));

  // 处理赋值操作
  if (binaryExpr->op == ast::BinaryExpr::Op::Assign) {
    // 检查左操作数是否是标识符（变量）
    if (auto *identifier =
            dynamic_cast<ast::Identifier *>(binaryExpr->left.get())) {
      const std::string &varName = identifier->name;

      // 检查是否是 late 变量
      auto it = lateVariables_.find(varName);
      if (it != lateVariables_.end()) {
        // 标记为已初始化
        it->second.isInitialized = true;
      }
    }
  }

  // 检查类型兼容性
  if (leftType && rightType) {
    if (!isTypeCompatible(leftType, rightType)) {
      error("type mismatch in binary expression", *binaryExpr);
    }
  }

  return leftType;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeIdentifierExpr(
    std::unique_ptr<ast::Identifier> identifier) {
  const std::string &name = identifier->name;

  // 检查是否是 late 变量
  auto it = lateVariables_.find(name);
  if (it != lateVariables_.end()) {
    const LateVariableStatus &status = it->second;
    if (!status.isInitialized) {
      error("use of uninitialized late variable '" + name + "'", *identifier);
    }
  }

  // 从符号表获取类型
  auto symbol = symbolTable.lookupSymbol(name);
  if (auto varSymbol = std::dynamic_pointer_cast<VariableSymbol>(symbol)) {
    return varSymbol->getType();
  }
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeLiteralExpr(std::unique_ptr<ast::Literal> literal) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeCallExpr(std::unique_ptr<ast::CallExpr> callExpr) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeMemberExpr(
    std::unique_ptr<ast::MemberExpr> memberExpr) {
  // 分析对象表达式
  std::shared_ptr<types::Type> objType =
      analyzeExpression(std::move(memberExpr->object));

  // 检查对象是否是 late 变量
  if (auto *identifier =
          dynamic_cast<ast::Identifier *>(memberExpr->object.get())) {
    const std::string &varName = identifier->name;
    auto it = lateVariables_.find(varName);
    if (it != lateVariables_.end()) {
      const LateVariableStatus &status = it->second;
      if (!status.isInitialized) {
        error("use of uninitialized late variable '" + varName + "'",
              *memberExpr);
      }
    }
  }

  return objType;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeSubscriptExpr(
    std::unique_ptr<ast::SubscriptExpr> subscriptExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeNewExpr(std::unique_ptr<ast::NewExpr> newExpr) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeDeleteExpr(
    std::unique_ptr<ast::DeleteExpr> deleteExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeThisExpr(std::unique_ptr<ast::ThisExpr> thisExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSuperExpr(std::unique_ptr<ast::SuperExpr> superExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSelfExpr(std::unique_ptr<ast::SelfExpr> selfExpr) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeExpansionExpr(
    std::unique_ptr<ast::ExpansionExpr> expansionExpr) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeLambdaExpr(
    std::unique_ptr<ast::LambdaExpr> lambdaExpr) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeArrayInitExpr(
    std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeStructInitExpr(
    std::unique_ptr<ast::StructInitExpr> structInitExpr) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTupleExpr(std::unique_ptr<ast::TupleExpr> tupleExpr) {
  return nullptr;
}

std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeType(const ast::Type *type) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzePrimitiveType(
    const ast::PrimitiveType *primitiveType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzePointerType(const ast::PointerType *pointerType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeArrayType(const ast::ArrayType *arrayType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeSliceType(const ast::SliceType *sliceType) {
  return nullptr;
}
std::shared_ptr<types::Type> SemanticAnalyzer::analyzeReferenceType(
    const ast::ReferenceType *referenceType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeFunctionType(const ast::FunctionType *functionType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeNamedType(const ast::NamedType *namedType) {
  return nullptr;
}
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeTupleType(const ast::TupleType *tupleType) {
  return nullptr;
}

bool SemanticAnalyzer::isLValue(const ast::Expression &expr) const {
  return false;
}
bool SemanticAnalyzer::isTypeCompatible(
    const std::shared_ptr<types::Type> &expected,
    const std::shared_ptr<types::Type> &actual) {
  return false;
}
std::shared_ptr<types::Type> SemanticAnalyzer::tryImplicitConversion(
    const std::shared_ptr<types::Type> &expected,
    const std::shared_ptr<types::Type> &actual) {
  return nullptr;
}
Visibility
SemanticAnalyzer::parseVisibility(const std::vector<std::string> &specifiers) {
  return Visibility::Default;
}
void SemanticAnalyzer::error(const std::string &message,
                             const ast::Node &node) {}
void SemanticAnalyzer::initializeBuiltinSymbols() {}

} // namespace semantic
} // namespace c_hat
