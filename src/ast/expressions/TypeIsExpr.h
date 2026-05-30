#pragma once

#include "Expression.h"
#include "../types/Type.h"
#include <memory>
#include <string>

namespace c_hat {
namespace ast {

class TypeIsExpr : public Expression {
public:
  enum class Kind {
    Struct,
    Class,
    Enum,
    Interface,
    Integer,
    Float,
    Bool,
    Char,
    String,
    Pointer,
    Reference,
    Slice,
    Array,
    Tuple,
    Function,
    Nullable,
    Primitive
  };

  TypeIsExpr(std::unique_ptr<Type> type, Kind kind)
      : type(std::move(type)), kind(kind) {}

  NodeType getType() const override { return NodeType::TypeIsExpr; }

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<TypeIsExpr>(
        std::unique_ptr<Type>(type->clone()), kind);
  }

  std::string toString() const override {
    std::string kindStr;
    switch (kind) {
    case Kind::Struct: kindStr = "struct"; break;
    case Kind::Class: kindStr = "class"; break;
    case Kind::Enum: kindStr = "enum"; break;
    case Kind::Interface: kindStr = "interface"; break;
    case Kind::Integer: kindStr = "integer"; break;
    case Kind::Float: kindStr = "float"; break;
    case Kind::Bool: kindStr = "bool"; break;
    case Kind::Char: kindStr = "char"; break;
    case Kind::String: kindStr = "string"; break;
    case Kind::Pointer: kindStr = "pointer"; break;
    case Kind::Reference: kindStr = "reference"; break;
    case Kind::Slice: kindStr = "slice"; break;
    case Kind::Array: kindStr = "array"; break;
    case Kind::Tuple: kindStr = "tuple"; break;
    case Kind::Function: kindStr = "function"; break;
    case Kind::Nullable: kindStr = "nullable"; break;
    case Kind::Primitive: kindStr = "primitive"; break;
    }
    return type->toString() + " is " + kindStr;
  }

  std::unique_ptr<Type> type;
  Kind kind;
};

} // namespace ast
} // namespace c_hat
