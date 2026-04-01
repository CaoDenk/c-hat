#include "CodeGenerator.h"
#include "../ast/types/ArrayType.h"
#include "../ast/types/NamedType.h"
#include "../ast/types/PointerType.h"
#include "../ast/types/PrimitiveType.h"
#include "../ast/types/SliceType.h"
#include <format>

namespace c_hat {
namespace codegen {

CodeGenerator::CodeGenerator() : indentLevel(0) {}

std::string CodeGenerator::generate(std::unique_ptr<ast::Program> program) {
  std::string code;
  for (auto &declaration : program->declarations) {
    code += generateDeclaration(std::move(declaration));
    code += "\n";
  }
  return code;
}

std::string CodeGenerator::generateDeclaration(
    std::unique_ptr<ast::Declaration> declaration) {
  switch (declaration->getType()) {
  case ast::NodeType::VariableDecl:
    return generateVariableDecl(std::unique_ptr<ast::VariableDecl>(
        static_cast<ast::VariableDecl *>(declaration.release())));
  case ast::NodeType::FunctionDecl:
    return generateFunctionDecl(std::unique_ptr<ast::FunctionDecl>(
        static_cast<ast::FunctionDecl *>(declaration.release())));
  case ast::NodeType::ClassDecl:
    return generateClassDecl(std::unique_ptr<ast::ClassDecl>(
        static_cast<ast::ClassDecl *>(declaration.release())));
  case ast::NodeType::TypeAliasDecl:
    return generateTypeAliasDecl(std::unique_ptr<ast::TypeAliasDecl>(
        static_cast<ast::TypeAliasDecl *>(declaration.release())));
  default:
    return "";
  }
}

std::string CodeGenerator::generateVariableDecl(
    std::unique_ptr<ast::VariableDecl> varDecl) {
  std::string code = indent();
  if (varDecl->isLate) {
    code += "late ";
  }
  if (varDecl->kind == ast::VariableKind::Var) {
    code += "var ";
  } else if (varDecl->kind == ast::VariableKind::Let) {
    code += "let ";
  } else if (varDecl->type) {
    code += generateType(std::unique_ptr<ast::Type>(
        static_cast<ast::Type *>(varDecl->type.release())));
    code += " ";
  }
  code += varDecl->name;
  if (varDecl->initializer) {
    code += " = ";
    code += generateExpression(std::move(varDecl->initializer));
  }
  code += ";";
  return code;
}

std::string CodeGenerator::generateFunctionDecl(
    std::unique_ptr<ast::FunctionDecl> funcDecl) {
  std::string code = indent();
  // TODO: 生成函数修饰符
  // TODO: 生成返回类型
  code += "auto ";
  code += funcDecl->name;
  code += "(";
  // TODO: 生成参数
  code += ")";
  if (funcDecl->body) {
    code += " ";
    code += generateStatement(std::unique_ptr<ast::Statement>(
        static_cast<ast::Statement *>(funcDecl->body.release())));
  } else if (funcDecl->arrowExpr) {
    code += " -> ";
    code += generateExpression(std::move(funcDecl->arrowExpr));
  }
  return code;
}

std::string
CodeGenerator::generateClassDecl(std::unique_ptr<ast::ClassDecl> classDecl) {
  std::string code = indent();
  // TODO: 生成类修饰符
  code += "class ";
  code += classDecl->name;
  if (!classDecl->baseClass.empty()) {
    code += " : public ";
    code += classDecl->baseClass;
  }
  code += " {\n";
  indentLevel++;
  // 生成类成员
  for (auto &member : classDecl->members) {
    switch (member->getType()) {
    case ast::NodeType::VariableDecl:
      code += generateVariableDecl(std::unique_ptr<ast::VariableDecl>(
          static_cast<ast::VariableDecl *>(member.release())));
      code += "\n";
      break;
    case ast::NodeType::FunctionDecl:
      code += generateFunctionDecl(std::unique_ptr<ast::FunctionDecl>(
          static_cast<ast::FunctionDecl *>(member.release())));
      code += "\n";
      break;
    default:
      // 其他类型的成员，暂不处理
      break;
    }
  }
  indentLevel--;
  code += indent();
  code += "}";
  return code;
}

std::string
CodeGenerator::generateStatement(std::unique_ptr<ast::Statement> statement) {
  switch (statement->getType()) {
  case ast::NodeType::VariableDecl:
    return generateVariableDecl(std::unique_ptr<ast::VariableDecl>(
        static_cast<ast::VariableStmt *>(statement.release())
            ->declaration.release()));
  case ast::NodeType::ExprStmt:
    return generateExprStmt(std::unique_ptr<ast::ExprStmt>(
        static_cast<ast::ExprStmt *>(statement.release())));
  case ast::NodeType::CompoundStmt:
    return generateCompoundStmt(std::unique_ptr<ast::CompoundStmt>(
        static_cast<ast::CompoundStmt *>(statement.release())));
  case ast::NodeType::ReturnStmt:
    return generateReturnStmt(std::unique_ptr<ast::ReturnStmt>(
        static_cast<ast::ReturnStmt *>(statement.release())));
  case ast::NodeType::ComptimeStmt:
    return generateComptimeStmt(std::unique_ptr<ast::ComptimeStmt>(
        static_cast<ast::ComptimeStmt *>(statement.release())));
  case ast::NodeType::IfStmt:
    return generateIfStmt(std::unique_ptr<ast::IfStmt>(
        static_cast<ast::IfStmt *>(statement.release())));
  case ast::NodeType::WhileStmt:
    return generateWhileStmt(std::unique_ptr<ast::WhileStmt>(
        static_cast<ast::WhileStmt *>(statement.release())));
  case ast::NodeType::ForStmt:
    return generateForStmt(std::unique_ptr<ast::ForStmt>(
        static_cast<ast::ForStmt *>(statement.release())));
  case ast::NodeType::BreakStmt:
    return generateBreakStmt(std::unique_ptr<ast::BreakStmt>(
        static_cast<ast::BreakStmt *>(statement.release())));
  case ast::NodeType::ContinueStmt:
    return generateContinueStmt(std::unique_ptr<ast::ContinueStmt>(
        static_cast<ast::ContinueStmt *>(statement.release())));
  case ast::NodeType::MatchStmt:
    return generateMatchStmt(std::unique_ptr<ast::MatchStmt>(
        static_cast<ast::MatchStmt *>(statement.release())));
  case ast::NodeType::TryStmt:
    return generateTryStmt(std::unique_ptr<ast::TryStmt>(
        static_cast<ast::TryStmt *>(statement.release())));
  case ast::NodeType::ThrowStmt:
    return generateThrowStmt(std::unique_ptr<ast::ThrowStmt>(
        static_cast<ast::ThrowStmt *>(statement.release())));
  case ast::NodeType::DeferStmt:
    return generateDeferStmt(std::unique_ptr<ast::DeferStmt>(
        static_cast<ast::DeferStmt *>(statement.release())));
  default:
    return "";
  }
}

std::string
CodeGenerator::generateExprStmt(std::unique_ptr<ast::ExprStmt> exprStmt) {
  std::string code = indent();
  code += generateExpression(std::move(exprStmt->expr));
  code += ";";
  return code;
}

std::string CodeGenerator::generateCompoundStmt(
    std::unique_ptr<ast::CompoundStmt> compoundStmt) {
  std::string code = "{\n";
  indentLevel++;
  for (auto &statement : compoundStmt->statements) {
    code += generateStatement(std::unique_ptr<ast::Statement>(
        static_cast<ast::Statement *>(statement.release())));
    code += "\n";
  }
  indentLevel--;
  code += indent();
  code += "}";
  return code;
}

std::string CodeGenerator::generateTypeAliasDecl(
    std::unique_ptr<ast::TypeAliasDecl> typeAliasDecl) {
  std::string code = indent();
  code += "typealias " + typeAliasDecl->name + " = ";
  code += generateType(std::move(typeAliasDecl->type));
  code += ";";
  return code;
}

std::string CodeGenerator::generateComptimeStmt(
    std::unique_ptr<ast::ComptimeStmt> comptimeStmt) {
  std::string code = indent();
  code += "comptime ";
  code += generateStatement(std::move(comptimeStmt->stmt));
  return code;
}

std::string
CodeGenerator::generateReturnStmt(std::unique_ptr<ast::ReturnStmt> returnStmt) {
  std::string code = indent();
  code += "return";
  if (returnStmt->expr) {
    code += " ";
    code += generateExpression(std::move(returnStmt->expr));
  }
  code += ";";
  return code;
}

std::string
CodeGenerator::generateExpression(std::unique_ptr<ast::Expression> expression) {
  switch (expression->getType()) {
  case ast::NodeType::BinaryExpr:
    return generateBinaryExpr(std::unique_ptr<ast::BinaryExpr>(
        static_cast<ast::BinaryExpr *>(expression.release())));
  case ast::NodeType::UnaryExpr:
    return generateUnaryExpr(std::unique_ptr<ast::UnaryExpr>(
        static_cast<ast::UnaryExpr *>(expression.release())));
  case ast::NodeType::Identifier:
    return generateIdentifierExpr(std::unique_ptr<ast::Identifier>(
        static_cast<ast::Identifier *>(expression.release())));
  case ast::NodeType::Literal:
    return generateLiteralExpr(std::unique_ptr<ast::Literal>(
        static_cast<ast::Literal *>(expression.release())));
  case ast::NodeType::CallExpr:
    return generateCallExpr(std::unique_ptr<ast::CallExpr>(
        static_cast<ast::CallExpr *>(expression.release())));
  case ast::NodeType::MemberExpr:
    return generateMemberExpr(std::unique_ptr<ast::MemberExpr>(
        static_cast<ast::MemberExpr *>(expression.release())));
  case ast::NodeType::SubscriptExpr:
    return generateSubscriptExpr(std::unique_ptr<ast::SubscriptExpr>(
        static_cast<ast::SubscriptExpr *>(expression.release())));
  case ast::NodeType::NewExpr:
    return generateNewExpr(std::unique_ptr<ast::NewExpr>(
        static_cast<ast::NewExpr *>(expression.release())));
  case ast::NodeType::SuperExpr:
    return generateSuperExpr(std::unique_ptr<ast::SuperExpr>(
        static_cast<ast::SuperExpr *>(expression.release())));
  case ast::NodeType::ExpansionExpr:
    return generateExpansionExpr(std::unique_ptr<ast::ExpansionExpr>(
        static_cast<ast::ExpansionExpr *>(expression.release())));
  case ast::NodeType::LambdaExpr:
    return generateLambdaExpr(std::unique_ptr<ast::LambdaExpr>(
        static_cast<ast::LambdaExpr *>(expression.release())));
  case ast::NodeType::TupleExpr:
    return generateTupleExpr(std::unique_ptr<ast::TupleExpr>(
        static_cast<ast::TupleExpr *>(expression.release())));
  case ast::NodeType::ArrayInitExpr:
    return generateArrayInitExpr(std::unique_ptr<ast::ArrayInitExpr>(
        static_cast<ast::ArrayInitExpr *>(expression.release())));
  case ast::NodeType::BuiltinVarExpr:
    return generateBuiltinVarExpr(std::unique_ptr<ast::BuiltinVarExpr>(
        static_cast<ast::BuiltinVarExpr *>(expression.release())));
  default:
    return "";
  }
}

std::string
CodeGenerator::generateBinaryExpr(std::unique_ptr<ast::BinaryExpr> binaryExpr) {
  std::string code = "(";
  code += generateExpression(std::move(binaryExpr->left));
  // TODO: 生成操作符
  code += " ";
  switch (binaryExpr->op) {
  case ast::BinaryExpr::Op::Add:
    code += "+";
    break;
  case ast::BinaryExpr::Op::Sub:
    code += "-";
    break;
  case ast::BinaryExpr::Op::Mul:
    code += "*";
    break;
  case ast::BinaryExpr::Op::Div:
    code += "/";
    break;
  case ast::BinaryExpr::Op::Mod:
    code += "%";
    break;
  case ast::BinaryExpr::Op::And:
    code += "&";
    break;
  case ast::BinaryExpr::Op::Or:
    code += "|";
    break;
  case ast::BinaryExpr::Op::Xor:
    code += "^";
    break;
  case ast::BinaryExpr::Op::Shl:
    code += "<<";
    break;
  case ast::BinaryExpr::Op::Shr:
    code += ">>";
    break;
  case ast::BinaryExpr::Op::Lt:
    code += "<";
    break;
  case ast::BinaryExpr::Op::Le:
    code += "<=";
    break;
  case ast::BinaryExpr::Op::Gt:
    code += ">";
    break;
  case ast::BinaryExpr::Op::Ge:
    code += ">=";
    break;
  case ast::BinaryExpr::Op::Eq:
    code += "==";
    break;
  case ast::BinaryExpr::Op::Ne:
    code += "!=";
    break;
  case ast::BinaryExpr::Op::LogicAnd:
    code += "&&";
    break;
  case ast::BinaryExpr::Op::LogicOr:
    code += "||";
    break;
  case ast::BinaryExpr::Op::Assign:
    code += "=";
    break;
  default:
    code += "";
    break;
  }
  code += " ";
  code += generateExpression(std::move(binaryExpr->right));
  code += ")";
  return code;
}

std::string
CodeGenerator::generateUnaryExpr(std::unique_ptr<ast::UnaryExpr> unaryExpr) {
  std::string code = "(";
  // TODO: 生成操作符
  switch (unaryExpr->op) {
  case ast::UnaryExpr::Op::Plus:
    code += "+";
    break;
  case ast::UnaryExpr::Op::Minus:
    code += "-";
    break;
  case ast::UnaryExpr::Op::Not:
    code += "!";
    break;
  case ast::UnaryExpr::Op::BitNot:
    code += "~";
    break;
  case ast::UnaryExpr::Op::AddressOf:
    code += "^";
    break;
  case ast::UnaryExpr::Op::Dereference:
    code += "^";
    break;
  case ast::UnaryExpr::Op::Await:
    code += "await ";
    break;
  case ast::UnaryExpr::Op::Move:
    code += "~";
    break;
  case ast::UnaryExpr::Op::Immutable:
    code += "!";
    break;
  case ast::UnaryExpr::Op::At:
    code += "@";
    break;
  default:
    code += "";
    break;
  }
  code += generateExpression(std::move(unaryExpr->expr));
  code += ")";
  return code;
}

std::string CodeGenerator::generateIdentifierExpr(
    std::unique_ptr<ast::Identifier> identifier) {
  return identifier->name;
}

std::string
CodeGenerator::generateLiteralExpr(std::unique_ptr<ast::Literal> literal) {
  return literal->value;
}

std::string
CodeGenerator::generateCallExpr(std::unique_ptr<ast::CallExpr> callExpr) {
  std::string code = generateExpression(std::move(callExpr->callee));
  code += "(";
  for (size_t i = 0; i < callExpr->args.size(); i++) {
    if (i > 0) {
      code += ", ";
    }
    code += generateExpression(std::move(callExpr->args[i]));
  }
  code += ")";
  return code;
}

std::string
CodeGenerator::generateMemberExpr(std::unique_ptr<ast::MemberExpr> memberExpr) {
  std::string code = generateExpression(std::move(memberExpr->object));
  // 暂时使用 . 操作符，后续可以根据类型系统改进，区分 . 和 ->
  code += ".";
  code += memberExpr->member;
  return code;
}

std::string CodeGenerator::generateSubscriptExpr(
    std::unique_ptr<ast::SubscriptExpr> subscriptExpr) {
  std::string code = generateExpression(std::move(subscriptExpr->object));
  code += "[";
  code += generateExpression(std::move(subscriptExpr->index));
  code += "]";
  return code;
}

std::string CodeGenerator::generateType(std::unique_ptr<ast::Type> type) {
  if (!type) {
    return "";
  }

  if (auto *primitiveType = dynamic_cast<ast::PrimitiveType *>(type.get())) {
    return primitiveType->toString();
  } else if (auto *namedType = dynamic_cast<ast::NamedType *>(type.get())) {
    return namedType->toString();
  } else if (auto *pointerType = dynamic_cast<ast::PointerType *>(type.get())) {
    std::string result = generateType(std::unique_ptr<ast::Type>(
        static_cast<ast::Type *>(pointerType->baseType.release())));
    result += "^";
    if (pointerType->isNullable) {
      result = "?" + result;
    }
    return result;
  } else if (auto *arrayType = dynamic_cast<ast::ArrayType *>(type.get())) {
    std::string result = generateType(std::unique_ptr<ast::Type>(
        static_cast<ast::Type *>(arrayType->baseType.release())));
    result += "[";
    if (arrayType->size) {
      result += generateExpression(
          std::unique_ptr<ast::Expression>(arrayType->size.release()));
    }
    result += "]";
    return result;
  } else if (auto *sliceType = dynamic_cast<ast::SliceType *>(type.get())) {
    std::string result = generateType(std::unique_ptr<ast::Type>(
        static_cast<ast::Type *>(sliceType->baseType.release())));
    result += "[]";
    return result;
  } else if (auto *referenceType =
                 dynamic_cast<ast::ReferenceType *>(type.get())) {
    std::string result = generateType(std::unique_ptr<ast::Type>(
        static_cast<ast::Type *>(referenceType->baseType.release())));
    result += "&";
    return result;
  }

  return "unknown";
}

std::string
CodeGenerator::generateNewExpr(std::unique_ptr<ast::NewExpr> newExpr) {
  std::string code = "new ";
  // 生成类型
  if (newExpr->type) {
    // 这里简化处理，假设类型是一个标识符
    if (auto *identifier =
            dynamic_cast<ast::Identifier *>(newExpr->type.get())) {
      code += identifier->name;
    }
  }
  // 生成参数列表
  code += "(";
  for (size_t i = 0; i < newExpr->args.size(); i++) {
    if (i > 0) {
      code += ", ";
    }
    code += generateExpression(std::move(newExpr->args[i]));
  }
  code += ")";
  return code;
}

std::string
CodeGenerator::generateSuperExpr(std::unique_ptr<ast::SuperExpr> superExpr) {
  return "super";
}

std::string CodeGenerator::generateExpansionExpr(
    std::unique_ptr<ast::ExpansionExpr> expansionExpr) {
  std::string code = generateExpression(std::move(expansionExpr->expr));
  code += "...";
  return code;
}

std::string CodeGenerator::generateBuiltinVarExpr(
    std::unique_ptr<ast::BuiltinVarExpr> builtinVarExpr) {
  const std::string &name = builtinVarExpr->name;

  if (name == "__line") {
    return "__LINE__";
  } else if (name == "__column") {
    return "__COLUMN__";
  } else if (name == "__file") {
    return "__FILE__";
  } else if (name == "__function") {
    return "__FUNCTION__";
  } else if (name == "__module") {
    return "\"__module__\"";
  } else if (name == "__compiler_version") {
    return "\"1.0.0\"";
  } else if (name == "__timestamp") {
    return "__TIMESTAMP__";
  } else if (name == "__build_mode") {
    return "0";
  }
  return name;
}

std::string CodeGenerator::indent() {
  return std::string(indentLevel * 2, ' ');
}

std::string CodeGenerator::generateIfStmt(std::unique_ptr<ast::IfStmt> ifStmt) {
  std::string code = indent();
  code += "if (";
  code += generateExpression(std::move(ifStmt->condition));
  code += ") ";
  code += generateStatement(std::move(ifStmt->thenBranch));
  if (ifStmt->elseBranch) {
    code += " else ";
    code += generateStatement(std::move(ifStmt->elseBranch));
  }
  return code;
}

std::string
CodeGenerator::generateWhileStmt(std::unique_ptr<ast::WhileStmt> whileStmt) {
  std::string code = indent();
  code += "while (";
  code += generateExpression(std::move(whileStmt->condition));
  code += ") ";
  code += generateStatement(std::move(whileStmt->body));
  return code;
}

std::string
CodeGenerator::generateForStmt(std::unique_ptr<ast::ForStmt> forStmt) {
  std::string code = indent();
  code += "for (";
  if (forStmt->init) {
    // 检查init是否是语句
    if (auto *stmt = dynamic_cast<ast::Statement *>(forStmt->init.get())) {
      code += generateStatement(std::unique_ptr<ast::Statement>(stmt));
      forStmt->init.release();
    } else {
      // 简化处理，输出占位符
      code += "/* init */";
    }
  }
  code += "; ";
  if (forStmt->condition) {
    code += generateExpression(std::move(forStmt->condition));
  }
  code += "; ";
  if (forStmt->update) {
    code += generateExpression(std::move(forStmt->update));
  }
  code += ") ";
  code += generateStatement(std::move(forStmt->body));
  return code;
}

std::string
CodeGenerator::generateBreakStmt(std::unique_ptr<ast::BreakStmt> breakStmt) {
  std::string code = indent();
  code += "break";
  code += ";";
  return code;
}

std::string CodeGenerator::generateContinueStmt(
    std::unique_ptr<ast::ContinueStmt> continueStmt) {
  std::string code = indent();
  code += "continue";
  code += ";";
  return code;
}

std::string
CodeGenerator::generateMatchStmt(std::unique_ptr<ast::MatchStmt> matchStmt) {
  std::string code = indent();
  code += "switch (";
  code += generateExpression(std::move(matchStmt->expr));
  code += ") {\n";
  indentLevel++;
  for (auto &arm : matchStmt->arms) {
    code += indent();
    code += "case ";
    // 简化处理，假设pattern是表达式
    code += "/* pattern */";
    code += ":\n";
    indentLevel++;
    code += generateStatement(std::move(arm->body));
    code += "\n";
    indentLevel--;
  }
  indentLevel--;
  code += indent();
  code += "}";
  return code;
}

std::string
CodeGenerator::generateTryStmt(std::unique_ptr<ast::TryStmt> tryStmt) {
  std::string code = indent();
  code += "try ";
  code += generateStatement(std::move(tryStmt->tryBlock));
  for (auto &catchStmt : tryStmt->catchStmts) {
    code += " catch (";
    code += "/* exception */";
    code += ") ";
    code += generateStatement(std::move(catchStmt->body));
  }
  return code;
}

std::string
CodeGenerator::generateThrowStmt(std::unique_ptr<ast::ThrowStmt> throwStmt) {
  std::string code = indent();
  code += "throw";
  if (throwStmt->expr) {
    code += " ";
    code += generateExpression(std::move(throwStmt->expr));
  }
  code += ";";
  return code;
}

std::string
CodeGenerator::generateDeferStmt(std::unique_ptr<ast::DeferStmt> deferStmt) {
  std::string code = indent();
  code += "defer ";
  code += generateExpression(std::move(deferStmt->expr));
  code += ";";
  return code;
}

std::string
CodeGenerator::generateLambdaExpr(std::unique_ptr<ast::LambdaExpr> lambdaExpr) {
  std::string code = "[";

  for (size_t i = 0; i < lambdaExpr->captures.size(); ++i) {
    if (i > 0) {
      code += ", ";
    }
    if (lambdaExpr->captures[i].byRef) {
      code += "&";
    }
    if (lambdaExpr->captures[i].isMove) {
      code += "= ";
    }
    code += lambdaExpr->captures[i].name;
  }

  code += "]";

  code += "(";
  for (size_t i = 0; i < lambdaExpr->params.size(); ++i) {
    if (i > 0) {
      code += ", ";
    }
    if (lambdaExpr->params[i]->type) {
      code += lambdaExpr->params[i]->type->toString() + " ";
    }
    code += lambdaExpr->params[i]->name;
  }
  code += ")";

  if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(lambdaExpr->body.get())) {
    code += " => ";
    code += generateExpression(
        std::unique_ptr<ast::Expression>(exprStmt->expr.release()));
  } else {
    code += " {";
    indentLevel++;
    code += "\n";

    if (auto *compoundStmt =
            dynamic_cast<ast::CompoundStmt *>(lambdaExpr->body.get())) {
      for (size_t i = 0; i < compoundStmt->statements.size(); ++i) {
        auto node = std::move(compoundStmt->statements[i]);
        if (auto *stmt = dynamic_cast<ast::Statement *>(node.get())) {
          node.release();
          code += generateStatement(std::unique_ptr<ast::Statement>(stmt));
        }
      }
    } else {
      code += generateStatement(std::move(lambdaExpr->body));
    }

    indentLevel--;
    code += indent() + "}";
  }

  return code;
}

std::string
CodeGenerator::generateTupleExpr(std::unique_ptr<ast::TupleExpr> tupleExpr) {
  std::string code = "std::make_tuple(";
  for (size_t i = 0; i < tupleExpr->elements.size(); ++i) {
    if (i > 0) {
      code += ", ";
    }
    code += generateExpression(std::move(tupleExpr->elements[i]));
  }
  code += ")";
  return code;
}

std::string CodeGenerator::generateArrayInitExpr(
    std::unique_ptr<ast::ArrayInitExpr> arrayInitExpr) {
  std::string code = "{";
  for (size_t i = 0; i < arrayInitExpr->elements.size(); ++i) {
    if (i > 0) {
      code += ", ";
    }
    code += generateExpression(std::move(arrayInitExpr->elements[i]));
  }
  code += "}";
  return code;
}

} // namespace codegen
} // namespace c_hat
