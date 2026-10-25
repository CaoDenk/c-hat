#pragma once

#include "AstNodes.h"
#include "types/ArrayType.h"
#include "types/FunctionType.h"
#include "types/GenericType.h"
#include "types/NamedType.h"
#include "types/NullableType.h"
#include "types/PointerType.h"
#include "types/PrimitiveType.h"
#include "types/ReadonlyType.h"
#include "types/ReferenceType.h"
#include "types/SliceType.h"
#include "types/TupleType.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace c_hat {
namespace ast {

// 泛型单态化使用的 AST 克隆/类型替换工具。
// TypeSubst 的 value 是调用点实际类型的 AST 节点（生命周期覆盖克隆过程）。
using TypeSubst = std::unordered_map<std::string, const Type *>;

std::unique_ptr<Type> cloneTypeWithSubst(const Type *type,
                                         const TypeSubst &subst);
std::unique_ptr<Expression> cloneExprWithSubst(const Expression *expr,
                                               const TypeSubst &subst);
std::unique_ptr<Node> cloneNodeWithSubst(const Node *node,
                                         const TypeSubst &subst);

std::unique_ptr<FunctionDecl>
cloneFunctionWithSubst(const FunctionDecl *func, const TypeSubst &subst,
                       const std::string &newName);

std::unique_ptr<ClassDecl>
cloneClassWithSubst(const ClassDecl *cls, const TypeSubst &subst,
                    const std::string &newName);

std::unique_ptr<StructDecl>
cloneStructWithSubst(const StructDecl *st, const TypeSubst &subst,
                     const std::string &newName);

} // namespace ast
} // namespace c_hat
