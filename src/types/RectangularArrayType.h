#pragma once

#include "Type.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace c_hat {
namespace types {

// 矩形数组类型（多维数组）
class RectangularArrayType : public Type {
public:
  RectangularArrayType(std::shared_ptr<Type> elementType,
                       std::vector<size_t> sizes);

  std::string toString() const override;
  bool isArray() const override { return true; }

  std::shared_ptr<Type> getElementType() const { return elementType; }
  const std::vector<size_t> &getSizes() const { return sizes; }
  int getRank() const { return static_cast<int>(sizes.size()); }

protected:
  bool isCompatibleWithImpl(const Type &other) const override;
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> elementType;
  std::vector<size_t> sizes;
};

} // namespace types
} // namespace c_hat
