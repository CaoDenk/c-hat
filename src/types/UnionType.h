#pragma once

#include "Type.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace types {

// 联合类型（类型集合），用于表示 using X = int | long | float
// 代表"这些类型中的任意一个"
class UnionType : public Type {
public:
  UnionType(std::vector<std::shared_ptr<Type>> memberTypes);

  std::string toString() const override;

  bool isUnion() const override { return true; }

  const std::vector<std::shared_ptr<Type>> &getMemberTypes() const {
    return memberTypes;
  }

protected:
  bool isCompatibleWithImpl(const Type &other) const override;
  bool isSubtypeOfImpl(const Type &other) const override;

private:
  std::vector<std::shared_ptr<Type>> memberTypes;
};

} // namespace types
} // namespace c_hat
