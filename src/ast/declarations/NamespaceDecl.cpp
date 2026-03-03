#include "NamespaceDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string NamespaceDecl::toString() const {
    std::string result = std::format("namespace {} {{\n", name);
    for (const auto& member : members) {
        result += "  " + member->toString() + "\n";
    }
    result += "}";
    return result;
}

} // namespace ast
} // namespace c_hat