#include "TypeFactory.h"

namespace c_hat {
namespace types {

std::shared_ptr<Type> TypeFactory::getPrimitiveType(PrimitiveType::Kind kind) {
  return std::make_shared<PrimitiveType>(kind);
}

std::shared_ptr<Type> TypeFactory::getPrimitiveTypeByName(const std::string &name) {
  if (name == "void") {
    return getPrimitiveType(PrimitiveType::Kind::Void);
  } else if (name == "bool") {
    return getPrimitiveType(PrimitiveType::Kind::Bool);
  } else if (name == "byte") {
    return getPrimitiveType(PrimitiveType::Kind::Byte);
  } else if (name == "sbyte") {
    return getPrimitiveType(PrimitiveType::Kind::SByte);
  } else if (name == "short") {
    return getPrimitiveType(PrimitiveType::Kind::Short);
  } else if (name == "ushort") {
    return getPrimitiveType(PrimitiveType::Kind::UShort);
  } else if (name == "int") {
    return getPrimitiveType(PrimitiveType::Kind::Int);
  } else if (name == "uint") {
    return getPrimitiveType(PrimitiveType::Kind::UInt);
  } else if (name == "long") {
    return getPrimitiveType(PrimitiveType::Kind::Long);
  } else if (name == "ulong") {
    return getPrimitiveType(PrimitiveType::Kind::ULong);
  } else if (name == "float") {
    return getPrimitiveType(PrimitiveType::Kind::Float);
  } else if (name == "double") {
    return getPrimitiveType(PrimitiveType::Kind::Double);
  } else if (name == "fp16") {
    return getPrimitiveType(PrimitiveType::Kind::Fp16);
  } else if (name == "bf16") {
    return getPrimitiveType(PrimitiveType::Kind::Bf16);
  } else if (name == "char") {
    return getPrimitiveType(PrimitiveType::Kind::Char);
  }
  return nullptr;
}

std::shared_ptr<Type>
TypeFactory::getArrayType(std::shared_ptr<Type> elementType, size_t size) {
  return std::make_shared<ArrayType>(elementType, size);
}

std::shared_ptr<Type>
TypeFactory::getSliceType(std::shared_ptr<Type> elementType) {
  return std::make_shared<SliceType>(elementType);
}

std::shared_ptr<Type>
TypeFactory::getRectangularArrayType(std::shared_ptr<Type> elementType,
                                     std::vector<size_t> sizes) {
  return std::make_shared<RectangularArrayType>(elementType, std::move(sizes));
}

std::shared_ptr<Type>
TypeFactory::getRectangularSliceType(std::shared_ptr<Type> elementType,
                                     int rank) {
  return std::make_shared<RectangularSliceType>(elementType, rank);
}

std::shared_ptr<Type>
TypeFactory::getPointerType(std::shared_ptr<Type> pointeeType,
                            bool isNullable) {
  return std::make_shared<PointerType>(pointeeType, isNullable);
}

std::shared_ptr<Type> TypeFactory::getFunctionType(
    std::shared_ptr<Type> returnType,
    std::vector<std::shared_ptr<Type>> parameterTypes) {
  return std::make_shared<FunctionType>(returnType, parameterTypes);
}

std::shared_ptr<Type> TypeFactory::getClassType(const std::string &name) {
  return std::make_shared<ClassType>(name);
}

std::shared_ptr<Type> TypeFactory::getInterfaceType(const std::string &name) {
  return std::make_shared<InterfaceType>(name);
}

std::shared_ptr<Type>
TypeFactory::getGenericType(const std::string &name,
                            std::vector<std::shared_ptr<Type>> typeArguments) {
  return std::make_shared<GenericType>(name, typeArguments);
}

std::shared_ptr<Type>
TypeFactory::getTupleType(std::vector<std::shared_ptr<Type>> elementTypes) {
  return std::make_shared<TupleType>(std::move(elementTypes));
}

std::shared_ptr<Type> TypeFactory::getLiteralViewType() {
  static std::shared_ptr<Type> instance = std::make_shared<LiteralViewType>();
  return instance;
}

std::shared_ptr<Type>
TypeFactory::getReadonlyType(std::shared_ptr<Type> baseType) {
  return std::make_shared<ReadonlyType>(std::move(baseType));
}

std::shared_ptr<Type>
TypeFactory::getReferenceType(std::shared_ptr<Type> baseType) {
  return std::make_shared<ReferenceType>(std::move(baseType));
}

} // namespace types
} // namespace c_hat
