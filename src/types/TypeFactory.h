#pragma once

#include "ArrayType.h"
#include "ClassType.h"
#include "FunctionType.h"
#include "GenericType.h"
#include "LiteralViewType.h"
#include "PointerType.h"
#include "PrimitiveType.h"
#include "ReadonlyType.h"
#include "RectangularArrayType.h"
#include "RectangularSliceType.h"
#include "SliceType.h"
#include "TupleType.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace types {

// 类型工厂
class TypeFactory {
public:
  // 获取基本类型
  static std::shared_ptr<Type> getPrimitiveType(PrimitiveType::Kind kind);

  // 获取数组类型
  static std::shared_ptr<Type> getArrayType(std::shared_ptr<Type> elementType,
                                            size_t size);

  // 获取切片类型
  static std::shared_ptr<Type> getSliceType(std::shared_ptr<Type> elementType);

  // 获取矩形数组类型
  static std::shared_ptr<Type>
  getRectangularArrayType(std::shared_ptr<Type> elementType,
                          std::vector<size_t> sizes);

  // 获取矩形切片类型
  static std::shared_ptr<Type>
  getRectangularSliceType(std::shared_ptr<Type> elementType, int rank);

  // 获取指针类型
  static std::shared_ptr<Type> getPointerType(std::shared_ptr<Type> pointeeType,
                                              bool isNullable = false);

  // 获取函数类型
  static std::shared_ptr<Type>
  getFunctionType(std::shared_ptr<Type> returnType,
                  std::vector<std::shared_ptr<Type>> parameterTypes);

  // 获取类类型
  static std::shared_ptr<Type> getClassType(const std::string &name);

  // 获取泛型类型
  static std::shared_ptr<Type>
  getGenericType(const std::string &name,
                 std::vector<std::shared_ptr<Type>> typeArguments);

  // 获取元组类型
  static std::shared_ptr<Type>
  getTupleType(std::vector<std::shared_ptr<Type>> elementTypes);

  // 获取 LiteralView 类型
  static std::shared_ptr<Type> getLiteralViewType();

  // 获取只读类型
  static std::shared_ptr<Type> getReadonlyType(std::shared_ptr<Type> baseType);
};

} // namespace types
} // namespace c_hat
