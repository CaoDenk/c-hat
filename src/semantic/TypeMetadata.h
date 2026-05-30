#pragma once

#include <string>
#include <vector>
#include <memory>

namespace c_hat {
namespace semantic {

struct FieldMetadata {
  std::string name;
  std::string typeName;
  size_t offset;
  bool isPublic;
  bool isMutable;
};

struct MethodMetadata {
  std::string name;
  std::string returnTypeName;
  size_t paramCount;
  bool isPublic;
  bool isStatic;
  bool isVirtual;
};

struct TypeMetadata {
  std::string name;
  size_t size;
  size_t align;
  bool isStruct;
  bool isClass;
  bool isEnum;
  bool isPrimitive;
  std::vector<FieldMetadata> fields;
  std::vector<MethodMetadata> methods;
};

class MetadataRegistry {
public:
  static MetadataRegistry &instance() {
    static MetadataRegistry registry;
    return registry;
  }

  void registerType(const std::string &name, TypeMetadata metadata) {
    types_[name] = std::move(metadata);
  }

  const TypeMetadata *getType(const std::string &name) const {
    auto it = types_.find(name);
    if (it != types_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  bool hasType(const std::string &name) const {
    return types_.find(name) != types_.end();
  }

private:
  MetadataRegistry() = default;
  std::unordered_map<std::string, TypeMetadata> types_;
};

} // namespace semantic
} // namespace c_hat
