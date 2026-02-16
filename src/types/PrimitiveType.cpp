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
  return isCompatibleWithImpl(other);
}

} // namespace types
} // namespace c_hat
