#include "UnionType.h"
#include <sstream>

namespace c_hat {
namespace types {

UnionType::UnionType(std::vector<std::shared_ptr<Type>> memberTypes)
    : memberTypes(std::move(memberTypes)) {}

std::string UnionType::toString() const {
  if (memberTypes.empty()) return "∅";
  std::ostringstream oss;
  for (size_t i = 0; i < memberTypes.size(); ++i) {
    if (i > 0) oss << " | ";
    oss << memberTypes[i]->toString();
  }
  return oss.str();
}

bool UnionType::isCompatibleWithImpl(const Type &other) const {
  // 如果 other 是相同联合类型，比较成员类型
  if (auto *otherUnion = dynamic_cast<const UnionType *>(&other)) {
    if (memberTypes.size() != otherUnion->memberTypes.size()) return false;
    for (size_t i = 0; i < memberTypes.size(); ++i) {
      if (!memberTypes[i]->isCompatibleWith(*otherUnion->memberTypes[i]))
        return false;
    }
    return true;
  }
  // 检查 other 是否是任一成员类型
  for (const auto &mt : memberTypes) {
    if (mt->isCompatibleWith(other)) return true;
  }
  return false;
}

bool UnionType::isSubtypeOfImpl(const Type &other) const {
  // 联合类型的每个成员都必须是 other 的子类型
  for (const auto &mt : memberTypes) {
    if (!mt->isSubtypeOf(other)) return false;
  }
  return true;
}

} // namespace types
} // namespace c_hat
