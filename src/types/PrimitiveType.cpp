#include "PrimitiveType.h"
#include <format>

namespace c_hat {
namespace types {

PrimitiveType::PrimitiveType(Kind kind) : kind(kind) {}

std::string PrimitiveType::toString() const {
  switch (kind) {
  case Kind::Void:
    return "void";
  case Kind::Bool:
    return "bool";
  case Kind::Byte:
    return "byte";
  case Kind::SByte:
    return "sbyte";
  case Kind::Short:
    return "short";
  case Kind::UShort:
    return "ushort";
  case Kind::Int:
    return "int";
  case Kind::UInt:
    return "uint";
  case Kind::Long:
    return "long";
  case Kind::ULong:
    return "ulong";
  case Kind::Float:
    return "float";
  case Kind::Double:
    return "double";
  case Kind::Fp16:
    return "fp16";
  case Kind::Bf16:
    return "bf16";
  case Kind::Char:
    return "char";
  default:
    return "unknown";
  }
}

bool PrimitiveType::isCompatibleWithImpl(const Type &other) const {
  if (!other.isPrimitive()) {
    return false;
  }
  const auto &otherPrimitive = static_cast<const PrimitiveType &>(other);
  return kind == otherPrimitive.kind;
}

bool PrimitiveType::isSubtypeOfImpl(const Type &other) const {
  if (!other.isPrimitive()) {
    return false;
  }

  const auto &otherPrimitive = static_cast<const PrimitiveType &>(other);

  // 相同类型，是子类型
  if (kind == otherPrimitive.kind) {
    return true;
  }

  // 整数类型之间的转换：可以从较小的整数类型转换到较大的整数类型
  if (isInteger() && otherPrimitive.isInteger()) {
    return getSize() <= otherPrimitive.getSize();
  }

  // 浮点数类型之间的转换：可以从较小的浮点数类型转换到较大的浮点数类型
  if (isFloatingPoint() && otherPrimitive.isFloatingPoint()) {
    return getSize() <= otherPrimitive.getSize();
  }

  // 整数到浮点数的转换：根据测试，整数类型不是浮点数类型的子类型
  // 即使它们之间可以进行隐式转换
  if (isInteger() && otherPrimitive.isFloatingPoint()) {
    return false;
  }

  return false;
}

bool PrimitiveType::isInteger() const {
  switch (kind) {
  case Kind::Byte:
  case Kind::SByte:
  case Kind::Short:
  case Kind::UShort:
  case Kind::Int:
  case Kind::UInt:
  case Kind::Long:
  case Kind::ULong:
  case Kind::Char:
    return true;
  default:
    return false;
  }
}

bool PrimitiveType::isFloatingPoint() const {
  switch (kind) {
  case Kind::Float:
  case Kind::Double:
  case Kind::Fp16:
  case Kind::Bf16:
    return true;
  default:
    return false;
  }
}

size_t PrimitiveType::getSize() const {
  switch (kind) {
  case Kind::Void:
    return 0;
  case Kind::Bool:
  case Kind::Byte:
  case Kind::SByte:
    return 1;
  case Kind::Short:
  case Kind::UShort:
  case Kind::Char:
  case Kind::Fp16:
  case Kind::Bf16:
    return 2;
  case Kind::Int:
  case Kind::UInt:
  case Kind::Float:
    return 4;
  case Kind::Long:
  case Kind::ULong:
  case Kind::Double:
    return 8;
  default:
    return 0;
  }
}

} // namespace types
} // namespace c_hat
