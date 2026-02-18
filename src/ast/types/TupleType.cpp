#include "TupleType.h"
#include <sstream>

namespace c_hat {
namespace ast {

std::string TupleType::toString() const {
    std::ostringstream oss;
    oss << "(";
    for (size_t i = 0; i < elementTypes.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << elementTypes[i]->toString();
    }
    oss << ")";
    return oss.str();
}

} // namespace ast
} // namespace c_hat
