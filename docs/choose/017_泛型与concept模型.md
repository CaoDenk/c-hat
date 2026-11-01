# 017：泛型与 Concept 模型

**状态**：草案  
**日期**：2026-11-01  
**范围**：泛型函数、泛型类/结构体、单态化、类型推导、concept、约束、关联类型、泛型 ABI  
**相关文档**：

- `docs/spec/C^ 语言泛型规范.md`
- `docs/design/泛型设计.md`
- `docs/choose/008_类型与代码生成边界.md`
- `docs/choose/010_OOP派发模型.md`
- `docs/choose/016_数组切片与元组值语义.md`

---

## 1. 背景

C^ 当前已经支持：

- 泛型函数：`id<int>`、`id(42)` 推导、递归泛型
- 泛型类：`Box<int>`、`Box<double>` 字段/构造/方法
- 泛型结构体：`Point<int>`、`Pair<int, double>`
- concept：`Addable` / `Comparable` 等内置概念，以及自定义 `requires`
- 类级 `where` 约束

但仍存在：

- 关联类型、嵌套 requires、复杂表达式不完整
- 推导只做简单一一对应
- 泛型类值构造 `Box<int>(...)` 不支持，仍建议 `new Box<int>(...)`
- 泛型 ABI / mangling / vtable 未完全冻结
- concept 错误信息不统一

需要先确定泛型模型，避免后续 ABI 和标准库反复改。

---

## 2. 决策

### 2.1 采用单态化（monomorphization）

- 泛型函数/类/结构体在调用点或实例化点单态化。
- 每个实际类型组合生成独立函数/类型。
- 名字使用稳定 mangling：
  - `id__int`
  - `Box__int`
  - `Point__int_double`
- 单态化后的类型进入 `SemanticModel` 和 `TypeLowering`。
- 不做类型擦除（v1 不采用 Java/C# 风格泛型）。
- 不做运行时泛型反射（v1）。

### 2.2 泛型函数

- 显式类型实参：`id<int>(42)`
- 简单推导：`id(42)`、`first(a)` 从 `T[]` 推导元素类型
- 支持递归泛型
- 支持函数重载与泛型组合
- 推导失败时给出明确错误，不默认 `int`

### 2.3 泛型类/结构体

- `class Box<T>`、`struct Point<T>` 在实例化时单态化。
- 字段、方法、构造函数、属性、静态成员都随实例化生成。
- 泛型类的 vtable 每个实例化独立生成。
- 泛型类值构造：
  - v1 推荐 `new Box<int>(...)`
  - `Box<int>(...)` 值构造列为后续支持
- 泛型结构体值构造：`Point<int>(...)` 支持。
- 泛型类/结构体的 ABI 按单态化后的具体类型确定。

### 2.4 类型参数与关联类型

- 类型参数写在名字之后：`func foo<T>`、`class Box<T>`。
- 关联类型用 `T.Item`，不使用 `T::Item`。
- 关联类型解析在 concept 约束和实例化时进行。
- v1 先支持简单关联类型；嵌套关联类型后续扩展。

### 2.5 Concept

Concept 语法：

```c^
concept Addable<T> = requires (T a, T b) {
    { a + b } -> T;
};
```

- concept 是编译期约束，不生成运行时对象。
- `where` 用于函数/类/结构体约束。
- 支持内置概念：`Addable`、`Comparable`、`Equatable`、`Copyable` 等。
- 支持自定义 `requires` 表达式：
  - 二元运算
  - 成员方法
  - 返回类型约束
  - 简单嵌套 requires
- concept 检查失败时给出具体原因，不只报“类型不满足”。

### 2.6 约束检查

- 函数调用时检查实参类型是否满足 concept。
- 类/结构体实例化时检查类级 `where`。
- 方法调用时检查 receiver 是否满足约束。
- 泛型递归实例化时避免无限展开。
- 约束检查发生在 semantic 阶段，codegen 不重复检查。

### 2.7 推导规则

v1 支持：

- 字面量推导：`id(42)` -> `T=int`
- 数组/切片元素推导：`first(a)` -> `T=element`
- 参数一一对应
- 显式类型实参优先于推导

v1 不支持：

- 从返回类型反推参数类型
- 从上下文类型反推
- 复杂模式匹配推导
- 高阶类型推导

推导失败时报错，不默认类型。

### 2.8 ABI 与 mangling

- 泛型函数/类/结构体单态化后使用具体类型 mangling。
- mangling 必须包含：
  - 泛型名
  - 类型实参
  - 模块路径（长期）
- 泛型类 vtable 每个实例化独立。
- 泛型实例化的符号必须稳定，供模块/标准库使用。
- 泛型实例化缓存：相同类型组合只生成一次。

### 2.9 编译期与运行时

- concept 检查在编译期完成。
- 单态化在编译期完成。
- v1 不做运行时泛型元数据。
- 反射/`typeof` 对泛型实例化的支持后续扩展。

---

## 3. 备选方案

### 方案 A：类型擦除

优点：代码体积小，运行时泛型统一。  
缺点：与 C^ 的零开销、显式类型、单态化方向冲突；需要运行时类型信息。

结论：不选。

### 方案 B：单态化（推荐）

优点：零开销、类型明确、与 LLVM 优化友好。  
缺点：代码体积可能增大；编译时间增加。

结论：选 B。

### 方案 C：混合（部分擦除）

优点：折中。  
缺点：复杂，ABI 不统一。

结论：v1 不选。

---

## 4. 当前实现状态

### 已实现

- 泛型函数单态化：`id<int>`、`id(42)`、递归
- 泛型类单态化：字段、构造、方法、类级 `where`
- 泛型结构体单态化：字段、方法、值构造
- 基础 concept：`Addable` / `Comparable` / 自定义 requires
- 简单类型推导

### 未实现

- 关联类型完整支持
- 嵌套 requires / 复杂表达式
- 泛型类值构造 `Box<int>(...)`
- 泛型 vtable
- 从返回类型/上下文推导
- 高阶类型/类型函数
- 泛型反射

---

## 5. 对现有代码的影响

主要涉及：

- `src/semantic/SemanticAnalyzer.*`
- `src/types/GenericType.*`
- `src/llvm/LLVMCodeGenerator.*`
- `src/ast/AstClone.*`
- `docs/spec/C^ 语言泛型规范.md`
- `docs/design/泛型设计.md`
- `tests/generics/`
- `tests/integration/IntegrationTest.cpp`

行为变化：

- 泛型类值构造可能需要支持
- concept 错误信息更具体
- 泛型 mangling 稳定化
- 泛型 vtable 与 OOP ADR 对齐
- 推导失败不再回退

---

## 6. 迁移计划

### Phase 0：冻结规则

- 单态化模型
- mangling
- 推导规则
- concept 检查范围
- 泛型类值构造策略

### Phase 1：semantic 完整化

- 关联类型
- 嵌套 requires
- 复杂表达式
- 约束错误信息
- 实例化缓存

### Phase 2：codegen/ABI

- 泛型实例化类型降低
- 泛型类 vtable
- 泛型结构体布局
- 跨模块泛型符号

### Phase 3：高级泛型

- 从返回类型/上下文推导
- 泛型类值构造
- 泛型反射
- 类型函数/高阶类型（如需要）

---

## 7. 待评审确认的问题

1. 是否坚持单态化，不做类型擦除？
2. 泛型类值构造 `Box<int>(...)` 是否进入 v1？
3. 关联类型是否进入 v1？
4. concept 是否支持嵌套 requires？
5. 推导是否只做简单一一对应？
6. 泛型 mangling 是否包含模块路径？
7. 泛型 vtable 是否每个实例化独立？
8. concept 错误信息需要多细？
9. 是否支持泛型递归实例化限制？
10. 是否支持泛型反射/typeof？

---

## 8. 验收标准

- 泛型函数/类/结构体单态化有端到端测试
- 显式类型实参与简单推导有测试
- concept 通过/拒绝有测试
- 关联类型有测试（若进入 v1）
- 泛型类值构造有测试（若进入 v1）
- 泛型 mangling 稳定
- 泛型 vtable 与 OOP ADR 一致
- 文档与实现一致
