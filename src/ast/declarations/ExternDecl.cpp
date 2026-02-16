#include "ExternDecl.h"
#include <format>

namespace c_hat {
namespace ast {

std::string ExternDecl::toString() const {
    std::string result = std::format("extern \"{}\" {{", abi);
    for (const auto& decl : declarations) {
        result += "\n  " + decl->toString();
    }
    result += "\n}";
    return result;
}

} // namespace ast
} // namespace c_hat
