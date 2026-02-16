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

std::string CodeGenerator::indent() {
  return std::string(indentLevel * 2, ' ');
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

} // namespace codegen
} // namespace c_hat
