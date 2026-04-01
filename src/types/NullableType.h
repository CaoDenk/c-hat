#pragma once

#include "Type.h"
#include <memory>

namespace c_hat {
namespace types {

class NullableType : public Type {
public:
  explicit NullableType(std::shared_ptr<Type> baseType);

  std::string toString() const override;
  bool isNullable() const override { return true; }
  std::shared_ptr<Type> getBaseType() const { return baseType; }

protected:
  bool isCompatibleWithImpl(const Type &other) const override;
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::shared_ptr<Type> baseType;
};

} // namespace types
} // namespace c_hat
