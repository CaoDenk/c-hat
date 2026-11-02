#pragma once

#include "Type.h"
#include <memory>
#include <string>

namespace c_hat {
namespace types {

class EnumType : public Type {
public:
  explicit EnumType(std::string name,
                    std::shared_ptr<Type> underlyingType = nullptr)
      : name_(std::move(name)), underlyingType_(std::move(underlyingType)) {}

  std::string toString() const override { return name_; }
  bool isEnum() const override { return true; }
  const std::string &getName() const { return name_; }
  std::shared_ptr<Type> getUnderlyingType() const { return underlyingType_; }

protected:
  bool isCompatibleWithImpl(const Type &other) const override {
    if (!other.isEnum()) {
      return false;
    }
    return name_ == static_cast<const EnumType &>(other).name_;
  }
  bool isSubtypeOfImpl(const Type &other) const override {
    return isCompatibleWithImpl(other);
  }

private:
  std::string name_;
  std::shared_ptr<Type> underlyingType_;
};

} // namespace types
} // namespace c_hat
