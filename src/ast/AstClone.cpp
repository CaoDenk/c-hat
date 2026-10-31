#include "AstClone.h"

#include "declarations/ClassDecl.h"
#include "declarations/FunctionDecl.h"
#include "declarations/StructDecl.h"
#include "declarations/VariableDecl.h"
#include "others/Parameter.h"
#include "statements/CompoundStmt.h"
#include "statements/DoWhileStmt.h"
#include "statements/ExprStmt.h"
#include "statements/ForStmt.h"
#include "statements/GotoStmt.h"
#include "statements/IfStmt.h"
#include "statements/LabelStmt.h"
#include "statements/ReturnStmt.h"
#include "statements/VariableStmt.h"
#include "statements/WhileStmt.h"
#include "statements/exception/DeferStmt.h"
#include "statements/exception/ThrowStmt.h"
#include "statements/exception/TryStmt.h"
#include "statements/match/MatchArm.h"
#include "statements/match/MatchStmt.h"
#include "statements/match/Pattern.h"

namespace c_hat {
namespace ast {

namespace {

std::unique_ptr<Node> cloneNodeImpl(const Node *node, const TypeSubst &subst);

std::unique_ptr<Type> cloneTypeImpl(const Type *type, const TypeSubst &subst);

std::unique_ptr<Expression> cloneExprImpl(const Expression *expr,
                                          const TypeSubst &subst);

std::unique_ptr<Statement> cloneStmtImpl(const Statement *stmt,
                                         const TypeSubst &subst);

std::unique_ptr<VariableDecl> cloneVariableDecl(const VariableDecl *decl,
                                                const TypeSubst &subst) {
  std::unique_ptr<Node> type;
  if (decl->type) {
    if (auto *typeNode = dynamic_cast<const Type *>(decl->type.get())) {
      type = cloneTypeImpl(typeNode, subst);
    }
  }
  std::unique_ptr<Expression> initializer;
  if (decl->initializer) {
    initializer = cloneExprImpl(decl->initializer.get(), subst);
  }
  return std::make_unique<VariableDecl>(
      decl->specifiers, decl->isLate, decl->kind, std::move(type), decl->name,
      std::move(initializer), decl->isConst, decl->isStatic);
}

std::unique_ptr<Parameter> cloneParameter(const Parameter *param,
                                          const TypeSubst &subst) {
  std::unique_ptr<Type> type;
  if (param->type) {
    type = cloneTypeImpl(param->type.get(), subst);
  }
  std::unique_ptr<Expression> defaultValue;
  if (param->defaultValue) {
    defaultValue = cloneExprImpl(param->defaultValue.get(), subst);
  }
  return std::make_unique<Parameter>(param->name, std::move(type),
                                     std::move(defaultValue), param->isVariadic,
                                     param->isSelf, param->isSelfImmutable);
}

std::unique_ptr<Pattern> clonePattern(const Pattern *pattern,
                                      const TypeSubst &subst) {
  auto cloned = std::make_unique<Pattern>();
  cloned->isDefault = pattern->isDefault;
  if (pattern->value) {
    cloned->value = cloneExprImpl(pattern->value.get(), subst);
  }
  return cloned;
}

std::unique_ptr<Expression> cloneExprImpl(const Expression *expr,
                                          const TypeSubst &subst) {
  if (auto *binary = dynamic_cast<const BinaryExpr *>(expr)) {
    return std::make_unique<BinaryExpr>(
        cloneExprImpl(binary->left.get(), subst), binary->op,
        cloneExprImpl(binary->right.get(), subst));
  }
  if (auto *unary = dynamic_cast<const UnaryExpr *>(expr)) {
    return std::make_unique<UnaryExpr>(
        unary->op, cloneExprImpl(unary->expr.get(), subst));
  }
  if (auto *call = dynamic_cast<const CallExpr *>(expr)) {
    std::vector<std::unique_ptr<Expression>> args;
    for (const auto &arg : call->args) {
      args.push_back(cloneExprImpl(arg.get(), subst));
    }
    return std::make_unique<CallExpr>(cloneExprImpl(call->callee.get(), subst),
                                      std::move(args));
  }
  if (auto *member = dynamic_cast<const MemberExpr *>(expr)) {
    auto cloned = std::make_unique<MemberExpr>(
        cloneExprImpl(member->object.get(), subst), member->member,
        member->isPointerMember);
    cloned->structName = member->structName;
    cloned->memberIndex = member->memberIndex;
    return cloned;
  }
  if (auto *subscript = dynamic_cast<const SubscriptExpr *>(expr)) {
    return std::make_unique<SubscriptExpr>(
        cloneExprImpl(subscript->object.get(), subst),
        cloneExprImpl(subscript->index.get(), subst));
  }
  if (auto *conditional = dynamic_cast<const ConditionalExpr *>(expr)) {
    return std::make_unique<ConditionalExpr>(
        cloneExprImpl(conditional->condition.get(), subst),
        cloneExprImpl(conditional->thenExpr.get(), subst),
        cloneExprImpl(conditional->elseExpr.get(), subst));
  }
  if (auto *tuple = dynamic_cast<const TupleExpr *>(expr)) {
    std::vector<std::unique_ptr<Expression>> elements;
    for (const auto &element : tuple->elements) {
      elements.push_back(cloneExprImpl(element.get(), subst));
    }
    return std::make_unique<TupleExpr>(std::move(elements));
  }
  if (auto *array = dynamic_cast<const ArrayInitExpr *>(expr)) {
    std::vector<std::unique_ptr<Expression>> elements;
    for (const auto &element : array->elements) {
      elements.push_back(cloneExprImpl(element.get(), subst));
    }
    return std::make_unique<ArrayInitExpr>(std::move(elements));
  }
  if (auto *newExpr = dynamic_cast<const NewExpr *>(expr)) {
    std::unique_ptr<Type> type;
    if (auto *typeNode = dynamic_cast<const Type *>(newExpr->type.get())) {
      type = cloneTypeImpl(typeNode, subst);
    }
    std::vector<std::unique_ptr<Expression>> args;
    for (const auto &arg : newExpr->args) {
      args.push_back(cloneExprImpl(arg.get(), subst));
    }
    return std::make_unique<NewExpr>(std::move(type), std::move(args));
  }
  if (auto *del = dynamic_cast<const DeleteExpr *>(expr)) {
    auto cloned =
        std::make_unique<DeleteExpr>(cloneExprImpl(del->expr.get(), subst),
                                     del->isArray);
    cloned->typeName = del->typeName;
    return cloned;
  }
  if (auto *ident = dynamic_cast<const Identifier *>(expr)) {
    std::vector<std::unique_ptr<Node>> templateArgs;
    for (const auto &arg : ident->templateArgs) {
      if (auto *argType = dynamic_cast<const Type *>(arg.get())) {
        templateArgs.push_back(cloneTypeImpl(argType, subst));
      }
    }
    return std::make_unique<Identifier>(ident->name, std::move(templateArgs));
  }
  if (auto *structInit = dynamic_cast<const StructInitExpr *>(expr)) {
    std::unique_ptr<Type> type;
    if (structInit->type) {
      type = cloneTypeImpl(structInit->type.get(), subst);
    }
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> fields;
    for (const auto &field : structInit->fields) {
      fields.push_back(
          {field.first, cloneExprImpl(field.second.get(), subst)});
    }
    return std::make_unique<StructInitExpr>(std::move(type),
                                            std::move(fields));
  }
  if (auto *typeIs = dynamic_cast<const TypeIsExpr *>(expr)) {
    return std::make_unique<TypeIsExpr>(
        cloneTypeImpl(typeIs->type.get(), subst), typeIs->kind);
  }
  if (dynamic_cast<const SelfExpr *>(expr)) {
    return std::make_unique<SelfExpr>();
  }
  if (dynamic_cast<const BaseExpr *>(expr)) {
    return std::make_unique<BaseExpr>();
  }
  // 其余表达式（Literal 等）没有类型参数，直接克隆。
  return expr->clone();
}

std::unique_ptr<Statement> cloneStmtImpl(const Statement *stmt,
                                         const TypeSubst &subst) {
  switch (stmt->getType()) {
  case NodeType::ReturnStmt: {
    auto *s = static_cast<const ReturnStmt *>(stmt);
    std::unique_ptr<Expression> expr;
    if (s->expr) {
      expr = cloneExprImpl(s->expr.get(), subst);
    }
    return std::make_unique<ReturnStmt>(std::move(expr));
  }
  case NodeType::ExprStmt: {
    auto *s = static_cast<const ExprStmt *>(stmt);
    return std::make_unique<ExprStmt>(
        cloneExprImpl(s->expr.get(), subst));
  }
  case NodeType::CompoundStmt: {
    auto *s = static_cast<const CompoundStmt *>(stmt);
    std::vector<std::unique_ptr<Node>> statements;
    for (const auto &child : s->statements) {
      statements.push_back(cloneNodeImpl(child.get(), subst));
    }
    return std::make_unique<CompoundStmt>(std::move(statements));
  }
  case NodeType::IfStmt: {
    auto *s = static_cast<const IfStmt *>(stmt);
    std::unique_ptr<Statement> elseBranch;
    if (s->elseBranch) {
      elseBranch = cloneStmtImpl(s->elseBranch.get(), subst);
    }
    return std::make_unique<IfStmt>(
        cloneExprImpl(s->condition.get(), subst),
        cloneStmtImpl(s->thenBranch.get(), subst), std::move(elseBranch));
  }
  case NodeType::WhileStmt: {
    auto *s = static_cast<const WhileStmt *>(stmt);
    return std::make_unique<WhileStmt>(
        cloneExprImpl(s->condition.get(), subst),
        cloneStmtImpl(s->body.get(), subst));
  }
  case NodeType::DoWhileStmt: {
    auto *s = static_cast<const DoWhileStmt *>(stmt);
    return std::make_unique<DoWhileStmt>(
        cloneStmtImpl(s->body.get(), subst),
        cloneExprImpl(s->condition.get(), subst));
  }
  case NodeType::ForStmt: {
    auto *s = static_cast<const ForStmt *>(stmt);
    std::unique_ptr<Node> init;
    if (s->init) {
      init = cloneNodeImpl(s->init.get(), subst);
    }
    std::unique_ptr<Expression> condition;
    if (s->condition) {
      condition = cloneExprImpl(s->condition.get(), subst);
    }
    std::unique_ptr<Expression> update;
    if (s->update) {
      update = cloneExprImpl(s->update.get(), subst);
    }
    auto cloned = std::make_unique<ForStmt>(
        std::move(init), std::move(condition), std::move(update),
        cloneStmtImpl(s->body.get(), subst));
    cloned->isForeach = s->isForeach;
    if (s->indexVar) {
      cloned->indexVar = cloneNodeImpl(s->indexVar.get(), subst);
    }
    return cloned;
  }
  case NodeType::BreakStmt:
    return std::make_unique<BreakStmt>();
  case NodeType::ContinueStmt:
    return std::make_unique<ContinueStmt>();
  case NodeType::GotoStmt: {
    auto *s = static_cast<const GotoStmt *>(stmt);
    return std::make_unique<GotoStmt>(s->label);
  }
  case NodeType::LabelStmt: {
    auto *s = static_cast<const LabelStmt *>(stmt);
    return std::make_unique<LabelStmt>(s->label);
  }
  case NodeType::ThrowStmt: {
    auto *s = static_cast<const ThrowStmt *>(stmt);
    std::unique_ptr<Expression> expr;
    if (s->expr) {
      expr = cloneExprImpl(s->expr.get(), subst);
    }
    auto cloned = std::make_unique<ThrowStmt>(std::move(expr));
    cloned->exceptionTag = s->exceptionTag;
    return cloned;
  }
  case NodeType::DeferStmt: {
    auto *s = static_cast<const DeferStmt *>(stmt);
    return std::make_unique<DeferStmt>(
        cloneExprImpl(s->expr.get(), subst));
  }
  case NodeType::TryStmt: {
    auto *s = static_cast<const TryStmt *>(stmt);
    std::vector<std::unique_ptr<CatchStmt>> catches;
    for (const auto &catchStmt : s->catchStmts) {
      std::unique_ptr<Parameter> param;
      if (catchStmt->param) {
        param = cloneParameter(catchStmt->param.get(), subst);
      }
      auto clonedCatch = std::make_unique<CatchStmt>(
          std::move(param), cloneStmtImpl(catchStmt->body.get(), subst));
      clonedCatch->catchTag = catchStmt->catchTag;
      catches.push_back(std::move(clonedCatch));
    }
    return std::make_unique<TryStmt>(
        cloneStmtImpl(s->tryBlock.get(), subst), std::move(catches));
  }
  case NodeType::MatchStmt: {
    auto *s = static_cast<const MatchStmt *>(stmt);
    std::vector<std::unique_ptr<MatchArm>> arms;
    for (const auto &arm : s->arms) {
      std::unique_ptr<Expression> guard;
      if (arm->guard) {
        guard = cloneExprImpl(arm->guard.get(), subst);
      }
      arms.push_back(std::make_unique<MatchArm>(
          clonePattern(arm->pattern.get(), subst), std::move(guard),
          cloneStmtImpl(arm->body.get(), subst)));
    }
    return std::make_unique<MatchStmt>(
        cloneExprImpl(s->expr.get(), subst), std::move(arms));
  }
  default:
    return nullptr;
  }
}

std::unique_ptr<Node> cloneNodeImpl(const Node *node, const TypeSubst &subst) {
  if (!node) {
    return nullptr;
  }
  if (auto *decl = dynamic_cast<const VariableDecl *>(node)) {
    return cloneVariableDecl(decl, subst);
  }
  if (auto *varStmt = dynamic_cast<const VariableStmt *>(node)) {
    auto decl = cloneVariableDecl(varStmt->declaration.get(), subst);
    return std::make_unique<VariableStmt>(std::move(decl));
  }
  if (auto *stmt = dynamic_cast<const Statement *>(node)) {
    return cloneStmtImpl(stmt, subst);
  }
  if (auto *expr = dynamic_cast<const Expression *>(node)) {
    return cloneExprImpl(expr, subst);
  }
  return nullptr;
}

std::unique_ptr<Type> cloneTypeImpl(const Type *type, const TypeSubst &subst) {
  if (!type) {
    return nullptr;
  }
  if (auto *named = dynamic_cast<const NamedType *>(type)) {
    auto it = subst.find(named->name);
    if (it != subst.end() && it->second) {
      return cloneTypeImpl(it->second, subst);
    }
    return std::make_unique<NamedType>(named->name);
  }
  if (auto *primitive = dynamic_cast<const PrimitiveType *>(type)) {
    return std::make_unique<PrimitiveType>(primitive->kind);
  }
  if (auto *pointer = dynamic_cast<const PointerType *>(type)) {
    return std::make_unique<PointerType>(
        cloneTypeImpl(pointer->baseType.get(), subst), pointer->isNullable);
  }
  if (auto *reference = dynamic_cast<const ReferenceType *>(type)) {
    return std::make_unique<ReferenceType>(
        cloneTypeImpl(reference->baseType.get(), subst));
  }
  if (auto *slice = dynamic_cast<const SliceType *>(type)) {
    return std::make_unique<SliceType>(
        cloneTypeImpl(slice->baseType.get(), subst));
  }
  if (auto *readonly = dynamic_cast<const ReadonlyType *>(type)) {
    return std::make_unique<ReadonlyType>(
        cloneTypeImpl(readonly->baseType.get(), subst));
  }
  if (auto *nullable = dynamic_cast<const NullableType *>(type)) {
    return std::make_unique<NullableType>(
        cloneTypeImpl(nullable->baseType.get(), subst));
  }
  if (auto *array = dynamic_cast<const ArrayType *>(type)) {
    return std::make_unique<ArrayType>(
        cloneTypeImpl(array->baseType.get(), subst),
        cloneExprImpl(array->size.get(), subst));
  }
  if (auto *generic = dynamic_cast<const GenericType *>(type)) {
    std::vector<std::unique_ptr<Node>> arguments;
    for (const auto &arg : generic->arguments) {
      if (auto *argType = dynamic_cast<const Type *>(arg.get())) {
        arguments.push_back(cloneTypeImpl(argType, subst));
      }
    }
    return std::make_unique<GenericType>(generic->name, std::move(arguments));
  }
  if (auto *tuple = dynamic_cast<const TupleType *>(type)) {
    std::vector<std::unique_ptr<Type>> elements;
    for (const auto &element : tuple->elementTypes) {
      elements.push_back(cloneTypeImpl(element.get(), subst));
    }
    return std::make_unique<TupleType>(std::move(elements));
  }
  if (auto *function = dynamic_cast<const FunctionType *>(type)) {
    std::vector<std::unique_ptr<Type>> params;
    for (const auto &param : function->parameterTypes) {
      params.push_back(cloneTypeImpl(param.get(), subst));
    }
    return std::make_unique<FunctionType>(
        std::move(params), cloneTypeImpl(function->returnType.get(), subst));
  }
  return type->clone();
}

} // namespace

std::unique_ptr<Type> cloneTypeWithSubst(const Type *type,
                                         const TypeSubst &subst) {
  return cloneTypeImpl(type, subst);
}

std::unique_ptr<Expression> cloneExprWithSubst(const Expression *expr,
                                               const TypeSubst &subst) {
  return cloneExprImpl(expr, subst);
}

std::unique_ptr<Node> cloneNodeWithSubst(const Node *node,
                                         const TypeSubst &subst) {
  return cloneNodeImpl(node, subst);
}

std::unique_ptr<FunctionDecl>
cloneFunctionWithSubst(const FunctionDecl *func, const TypeSubst &subst,
                       const std::string &newName) {
  std::vector<std::unique_ptr<Node>> params;
  for (const auto &param : func->params) {
    if (auto *paramNode = dynamic_cast<const Parameter *>(param.get())) {
      params.push_back(cloneParameter(paramNode, subst));
    }
  }

  std::unique_ptr<Node> returnType;
  if (func->returnType) {
    if (auto *typeNode =
            dynamic_cast<const Type *>(func->returnType.get())) {
      returnType = cloneTypeImpl(typeNode, subst);
    }
  }

  std::unique_ptr<Node> body;
  if (func->body) {
    body = cloneNodeImpl(func->body.get(), subst);
  }

  std::unique_ptr<Expression> arrowExpr;
  if (func->arrowExpr) {
    arrowExpr = cloneExprImpl(func->arrowExpr.get(), subst);
  }

  std::unique_ptr<Expression> baseCall;
  if (func->baseCall) {
    baseCall = cloneExprImpl(func->baseCall.get(), subst);
  }

  return std::make_unique<FunctionDecl>(
      func->specifiers, newName,
      std::vector<std::unique_ptr<Node>>(), std::move(params),
      std::move(returnType), nullptr, nullptr, std::move(body),
      std::move(arrowExpr), func->isImmutable, std::move(baseCall),
      func->isVariadic, func->isStatic);
}

std::unique_ptr<ClassDecl>
cloneClassWithSubst(const ClassDecl *cls, const TypeSubst &subst,
                    const std::string &newName) {
  std::vector<std::unique_ptr<Node>> members;
  for (const auto &member : cls->members) {
    if (auto *varDecl = dynamic_cast<const VariableDecl *>(member.get())) {
      members.push_back(cloneVariableDecl(varDecl, subst));
    } else if (auto *funcDecl =
                   dynamic_cast<const FunctionDecl *>(member.get())) {
      std::string memberName = funcDecl->name;
      if (memberName == cls->name) {
        memberName = newName;
      } else if (memberName == "~" + cls->name) {
        memberName = "~" + newName;
      }
      members.push_back(
          cloneFunctionWithSubst(funcDecl, subst, memberName));
    }
  }

  return std::make_unique<ClassDecl>(
      cls->specifiers, newName, std::vector<std::unique_ptr<Node>>(),
      cls->baseClass, cls->baseClasses, cls->interfaces, std::move(members));
}

std::unique_ptr<StructDecl>
cloneStructWithSubst(const StructDecl *st, const TypeSubst &subst,
                     const std::string &newName) {
  std::vector<std::unique_ptr<Node>> members;
  for (const auto &member : st->members) {
    if (auto *varDecl = dynamic_cast<const VariableDecl *>(member.get())) {
      members.push_back(cloneVariableDecl(varDecl, subst));
    } else if (auto *funcDecl =
                   dynamic_cast<const FunctionDecl *>(member.get())) {
      std::string memberName = funcDecl->name;
      if (memberName == st->name) {
        memberName = newName;
      } else if (memberName == "~" + st->name) {
        memberName = "~" + newName;
      }
      members.push_back(
          cloneFunctionWithSubst(funcDecl, subst, memberName));
    }
  }

  return std::make_unique<StructDecl>(
      st->specifiers, newName, std::vector<std::unique_ptr<Node>>(),
      std::move(members));
}

} // namespace ast
} // namespace c_hat
