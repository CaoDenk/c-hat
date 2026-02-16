#include "ExtensionDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ExtensionDecl::toString() const {
    std::string result = "ExtensionDecl(";
    result += extendedType->toString();
    result += ", {";
    for (size_t i = 0; i < members.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += members[i]->toString();
    }
    result += "})";
    return result;
}

} // namespace ast
} // namespace c_hat
