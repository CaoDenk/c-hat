#pragma once

#include "Type.h"
#include <cstddef>
#include <memory>

namespace c_hat {
namespace types {

// 矩形切片类型（多维切片）
class RectangularSliceType : public Type {
public:
  RectangularSliceType(std::shared_ptr<Type> elementType, int rank);

  std::string toString() const override;
  bool isSlice() const override { return true; }

  std::shared_ptr<Type> getElementType() const { return elementType; }
  int getRank() const { return rank; }

protected:
  bool isCompatibleWithImpl(const Type &other) const override;
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> elementType;
  int rank;
};

} // namespace types
} // namespace c_hat
