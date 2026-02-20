#pragma once

#include "Type.h"

namespace c_hat {
namespace ast {

// 基本类型
class PrimitiveType : public Type {
public:
  enum class Kind {
    Void,
    Bool,
    Byte,
    SByte,
    Short,
    UShort,
    Int,
    UInt,
    Long,
    ULong,
    Float,
    Double,
    Fp16,
    Bf16,
    Char
  };

  PrimitiveType(Kind kind) : kind(kind) {}

  NodeType getType() const override { return NodeType::PrimitiveType; }
  std::string toString() const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<PrimitiveType>(kind);
  }

  Kind kind;
};

} // namespace ast
} // namespace c_hat
