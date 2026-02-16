#include "TypeFactory.h"

namespace c_hat {
namespace types {

std::shared_ptr<Type> TypeFactory::getPrimitiveType(PrimitiveType::Kind kind) {
  return std::make_shared<PrimitiveType>(kind);
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

} // namespace types
} // namespace c_hat
