# LiteralView 与只读类型系统进展

## 日期
2026-02-16

## 当前进展

### 1. LiteralView 类型实现

#### 1.1 设计文档完善
- **文件**: `docs/design/LiteralView设计.md`
- **内容**:
  - 明确了 `LiteralView.ptr` 和 `len` 是只读 get 属性
  - 添加注释说明这些属性不能被赋值
  - 完善了 `LiteralView` 的 ABI 布局和设计说明

#### 1.2 类型系统实现
- **新增文件**:
  - `src/types/LiteralViewType.h` - LiteralView 类型类头文件
  - `src/types/LiteralViewType.cpp` - LiteralView 类型实现
- **修改文件**:
  - `src/types/TypeFactory.h` - 添加 getLiteralViewType() 方法
  - `src/types/TypeFactory.cpp` - 实现 LiteralView 类型工厂
  - `src/ast/types/Type.h` - 添加 LiteralViewType AST 节点
  - `src/ast/types/Type.cpp` - 实现 LiteralViewType 的 toString
  - `src/parser/Parser.cpp` - 支持解析 `literalview` 关键字为类型
  - `src/semantic/SemanticAnalyzer.cpp` - 添加字符串字面量类型推导为 LiteralViewType

#### 1.3 功能特性
- ✅ `LiteralView` 作为内置原生类型
- ✅ 字符串字面量自动推导为 `LiteralView` 类型
- ✅ `LiteralView` 包含两个字段：`ptr` 和 `len`
- ✅ `ptr` 和 `len` 设计为只读属性（get）

### 2. 只读类型系统实现

#### 2.1 ReadonlyType 类型
- **新增文件**:
  - `src/types/ReadonlyType.h` - Readonly 类型类头文件
  - `src/types/ReadonlyType.cpp` - Readonly 类型实现
- **功能**:
  - 可以包装任意类型，使其成为只读类型
  - 语法表示为 `T!`（类型后缀加感叹号）
  - 支持 `isReadonly()` 检查
  - 支持 `getBaseType()` 获取原始类型

#### 2.2 赋值检查
- **实现位置**: `src/semantic/SemanticAnalyzer.cpp` - `analyzeBinaryExpr`
- **功能**:
  - 检查赋值表达式左操作数是否为只读类型
  - 如果是只读类型，报错 "Cannot assign to readonly type"

#### 2.3 类型兼容性规则
- **实现位置**: `src/types/Type.h` 和 `src/types/Type.cpp`
- **重构内容**:
  - 添加 `isCompatibleWithImpl` 和 `isSubtypeOfImpl` 作为纯虚函数
  - `isCompatibleWith` 和 `isSubtypeOf` 现在在基类中统一处理顶层 Readonly 兼容性
  - 具体类型实现 `isCompatibleWithImpl` 和 `isSubtypeOfImpl` 方法
  - 添加 `unwrapReadonly` 辅助函数，用于剥除 Readonly 包装

#### 2.4 顶层 Readonly 兼容性规则
- ✅ `T!` 与 `T` 兼容（值拷贝可以去掉顶层 const）
- ✅ `T` 与 `T!` 兼容（值拷贝可以增加顶层 const）
- ✅ `T!` 与 `T!` 兼容
- ✅ 赋值时左操作数不能是顶层 Readonly

### 3. 测试验证

#### 3.1 测试文件
- **新增文件**:
  - `tests/test_literalview.ch` - LiteralView 类型测试
  - `tests/test_readonly_compatibility.ch` - 只读类型兼容性测试
  - `tests/test_simple_pointer.ch` - 简单指针类型测试

#### 3.2 测试结果
- ✅ `test_literalview.ch` - LiteralView 类型解析和语义分析成功
- ✅ `test_readonly_compatibility.ch` - 只读类型兼容性检查正常工作
- ✅ `test_simple_pointer.ch` - 指针类型解析成功，包括 `byte!^`

---

## 技术细节

### 1. Type 基类重构

```cpp
// Type.h
class Type {
public:
  // 检查类型兼容性（处理顶层 Readonly）
  bool isCompatibleWith(const Type &other) const;

  // 检查子类型关系（处理顶层 Readonly）
  bool isSubtypeOf(const Type &other) const;

protected:
  // 具体类型的兼容性检查实现（不包含 Readonly 包装处理）
  virtual bool isCompatibleWithImpl(const Type &other) const = 0;

  // 具体类型的子类型关系检查实现（不包含 Readonly 包装处理）
  virtual bool isSubtypeOfImpl(const Type &other) const = 0;
};
```

### 2. Type.cpp 的实现

```cpp
// 处理顶层 Readonly 兼容性
bool Type::isCompatibleWith(const Type &other) const {
  if (this == &other) return true;
  
  // 处理顶层 Readonly
  if (this->isReadonly()) {
    const auto* readonlyThis = static_cast<const ReadonlyType*>(this);
    if (readonlyThis->getBaseType()->isCompatibleWith(other)) {
      return true;
    }
  }
  
  if (other.isReadonly()) {
    const auto* readonlyOther = static_cast<const ReadonlyType*>(&other);
    if (this->isCompatibleWith(*readonlyOther->getBaseType())) {
      return true;
    }
  }
  
  // 双方都是 Readonly
  if (this->isReadonly() && other.isReadonly()) {
    const auto* readonlyThis = static_cast<const ReadonlyType*>(this);
    const auto* readonlyOther = static_cast<const ReadonlyType*>(&other);
    if (readonlyThis->getBaseType()->isCompatibleWith(*readonlyOther->getBaseType())) {
      return true;
    }
  }
  
  // 双方都不是 Readonly
  if (!this->isReadonly() && !other.isReadonly()) {
    return this->isCompatibleWithImpl(other);
  }
  
  return false;
}
```

### 3. 语法解析顺序

1. 基础类型（如 `byte`）
2. `!` 符号 → ReadonlyType（顶层 const）
3. 类型后缀（如 `^`）→ PointerType

所以：
- `byte!^` → Pointer<Readonly<byte>>（底层 const，指向只读 byte）
- `byte^!` → Readonly<Pointer<byte>>（顶层 const，指针本身只读）

---

## 下一步计划

### 短期目标（1-2天）
1. **完善指针类型的兼容性检查**
   - [ ] 实现指针类型的特殊兼容性规则
   - [ ] `T^` 与 `T!^` 兼容（增加底层 const）
   - [ ] `T!^` 不与 `T^` 兼容（不能去掉底层 const）
   - [ ] 添加指针类型兼容性测试

2. **添加 LiteralView 到 byte![] 的隐式转换**
   - [ ] 在语义分析器中添加隐式转换支持
   - [ ] 测试 `byte![] s = "hello"` 是否工作

3. **完善 LiteralView 的代码生成**
   - [ ] 在 LLVMCodeGenerator 中添加 LiteralView 支持
   - [ ] 生成 .rodata 段的字符串字面量
   - [ ] 生成 ptr+len 的结构

### 中期目标（1-2周）
1. **完善字符串系统**
   - [ ] 实现 string 类型（标准库）
   - [ ] 实现 string_view 类型（标准库）
   - [ ] 实现字符串字面量到 string 的隐式转换

2. **完善数组和切片**
   - [ ] 完善切片类型的支持
   - [ ] 实现数组到切片的隐式转换
   - [ ] 实现切片遍历

3. **完善标准库基础**
   - [ ] 实现基础 I/O 函数
   - [ ] 实现字符串处理函数
   - [ ] 使用扩展机制实现内置类型的方法

---

## 已修改/新增文件清单

### 新增文件
1. `src/types/LiteralViewType.h`
2. `src/types/LiteralViewType.cpp`
3. `src/types/ReadonlyType.h`
4. `src/types/ReadonlyType.cpp`
5. `src/types/Type.cpp`
6. `tests/test_literalview.ch`
7. `tests/test_readonly_compatibility.ch`
8. `tests/test_simple_pointer.ch`

### 修改文件
1. `src/types/Type.h` - 添加 isCompatibleWithImpl 和 isSubtypeOfImpl
2. `src/types/TypeFactory.h` - 添加 getLiteralViewType()
3. `src/types/TypeFactory.cpp` - 实现 LiteralView 类型工厂
4. `src/types/PrimitiveType.cpp` - 重命名为 isCompatibleWithImpl
5. `src/types/ArrayType.cpp` - 重命名为 isCompatibleWithImpl
6. `src/types/RectangularArrayType.cpp` - 重命名为 isCompatibleWithImpl
7. `src/types/SliceType.cpp` - 重命名为 isCompatibleWithImpl
8. `src/types/RectangularSliceType.cpp` - 重命名为 isCompatibleWithImpl
9. `src/types/PointerType.cpp` - 重命名为 isCompatibleWithImpl
10. `src/types/TupleType.cpp` - 重命名为 isCompatibleWithImpl
11. `src/types/FunctionType.cpp` - 重命名为 isCompatibleWithImpl
12. `src/types/ClassType.cpp` - 重命名为 isCompatibleWithImpl
13. `src/types/GenericType.cpp` - 重命名为 isCompatibleWithImpl
14. `src/ast/types/Type.h` - 添加 LiteralViewType AST 节点
15. `src/ast/types/Type.cpp` - 实现 LiteralViewType 的 toString
16. `src/parser/Parser.cpp` - 支持解析 literalview 关键字
17. `src/semantic/SemanticAnalyzer.cpp` - 添加字符串字面量类型推导和只读检查
18. `src/ast/CMakeLists.txt` - 添加 LiteralViewType
19. `src/types/CMakeLists.txt` - 添加 LiteralViewType, ReadonlyType, Type.cpp
20. `docs/design/LiteralView设计.md` - 完善设计文档

---

## 总结

本次实现了两个核心功能：
1. **LiteralView 类型** - 用于承载字符串字面量的内置类型
2. **只读类型系统** - 完整的顶层 Readonly 支持，包括：
   - Readonly 类型包装
   - 赋值检查
   - 顶层 Readonly 兼容性规则
   - 类型系统框架重构

所有核心功能都已实现并通过了基本测试！
