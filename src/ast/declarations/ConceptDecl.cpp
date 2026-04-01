#include "ConceptDecl.h"
#include "../templates/TemplateParameter.h"
#include <sstream>

namespace c_hat {
namespace ast {

std::string ConceptDecl::toString() const {
  std::ostringstream oss;
  oss << "ConceptDecl(" << name;
  if (!templateParams.empty()) {
    oss << "<";
    for (size_t i = 0; i < templateParams.size(); ++i) {
      if (i > 0)
        oss << ", ";
      if (auto *param =
              dynamic_cast<TemplateParameter *>(templateParams[i].get())) {
        oss << param->name;
      }
    }
    oss << ">";
  }
  oss << ")";
  return oss.str();
}

} // namespace ast
} // namespace c_hat
