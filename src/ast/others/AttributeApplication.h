#pragma once

#include "../expressions/Expression.h"
#include <memory>
#include <string>
#include <vector>

namespace c_hat {
namespace ast {

class AttributeArgument {
public:
  std::string name;
  std::unique_ptr<Expression> value;

  AttributeArgument(const std::string &name, std::unique_ptr<Expression> value)
      : name(name), value(std::move(value)) {}

  AttributeArgument(std::unique_ptr<Expression> value)
      : name(""), value(std::move(value)) {}
};

class AttributeApplication {
public:
  std::string name;
  std::vector<std::unique_ptr<AttributeArgument>> arguments;

  AttributeApplication(const std::string &name) : name(name) {}

  void addArgument(std::unique_ptr<AttributeArgument> arg) {
    arguments.push_back(std::move(arg));
  }

  std::string toString() const {
    std::string result = "[" + name;
    if (!arguments.empty()) {
      result += "(";
      for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) {
          result += ", ";
        }
        if (!arguments[i]->name.empty()) {
          result += arguments[i]->name + " = ";
        }
        if (arguments[i]->value) {
          result += arguments[i]->value->toString();
        }
      }
      result += ")";
    }
    result += "]";
    return result;
  }
};

} // namespace ast
} // namespace c_hat
