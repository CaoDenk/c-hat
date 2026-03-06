# C-Hat 函数重载选择规则

## 概述

本文档描述 C-Hat 语言中函数重载的选择机制，特别是当多个重载函数都匹配传入参数时的处理策略。

## 问题场景

考虑以下函数重载：

```cpp
func process(data: byte![]) -> void;
func process(data: byte!^) -> void;

// 传入字符串字面量
process("hello");
```

字符串字面量在 C-Hat 中的默认类型是 `LiteralView`（通过标准库 `std.core.LiteralView` 定义）。`LiteralView` 提供了两个隐式转换：

1. `implicit operator byte![]()` - 转换为只读切片
2. `implicit operator byte!^()` - 转换为只读指针

## 当前设计决策

### 方案对比

| 方案 | 描述 | 优点 | 缺点 |
|------|------|------|------|
| **A. 编译报错（歧义）** | 当多个重载都匹配时，编译器报错 | 明确、可预测 | 需要显式类型转换 |
| **B. 优先级选择** | 定义隐式转换的优先级 | 更灵活 | 规则复杂，可能产生意外行为 |
| **C. 最佳匹配** | 选择"最精确"的匹配 | 符合直觉 | 需要定义"精确度"标准 |

### C-Hat 的选择：**方案 A（编译报错）**

C-Hat 采用**最保守的策略**：当存在多个同样有效的隐式转换路径时，编译器直接报错，要求程序员显式指定转换方式。

**理由：**
1. **避免意外行为**：隐式转换的优先级规则往往导致难以预料的结果
2. **代码清晰性**：强制显式转换使代码意图更加明确
3. **可维护性**：减少因隐式转换规则变化导致的代码破坏
4. **符合 C-Hat 设计理念**：显式优于隐式，安全优于便利

## 具体规则

### 规则 1：精确匹配优先

如果存在参数类型完全匹配的重载，优先选择精确匹配：

```cpp
func foo(x: int) -> void;
func foo(x: long) -> void;

foo(42);      // 选择 foo(int) - 精确匹配
foo(42L);     // 选择 foo(long) - 精确匹配
```

### 规则 2：单一隐式转换路径

如果只有一个重载可以通过隐式转换匹配，选择该重载：

```cpp
func bar(x: byte![]) -> void;

bar("hello"); // 选择 bar(byte![]) - 唯一可行路径
```

### 规则 3：歧义时报错

如果多个重载都可以通过隐式转换匹配，编译报错：

```cpp
func process(data: byte![]) -> void;
func process(data: byte!^) -> void;

process("hello");  // 编译错误：歧义的函数调用
                   // LiteralView 可以隐式转换为 byte![] 或 byte!^
                   // 无法确定应该选择哪个重载
```

**解决方案：**

```cpp
// 方案 1：显式类型转换
process("hello" as byte![]);  // 明确选择 byte![] 版本
process("hello" as byte!^);   // 明确选择 byte!^ 版本

// 方案 2：使用中间变量
let slice: byte![] = "hello";
process(slice);  // 明确匹配 byte![] 版本
```

## 隐式转换的优先级（内部实现）

虽然对外表现为"歧义时报错"，但内部实现仍有优先级概念：

### 标准库定义的隐式转换

标准库中的隐式转换通过 `implicit operator` 定义：

```cpp
// std/core/LiteralView.ch
extension LiteralView {
    // 转换为切片
    public implicit operator byte![]() {
        return { ptr = self.ptr, len = self.len };
    }
    
    // 转换为指针
    public implicit operator byte!^() {
        return self.ptr;
    }
}
```

### 编译器处理流程

1. **收集候选**：找出所有可能匹配的重载函数
2. **可行性检查**：检查每个候选是否可以通过隐式转换匹配
3. **歧义检测**：如果有多个可行候选，检查是否存在精确匹配
4. **错误报告**：如果存在多个同样可行的候选，报告歧义错误

## 与 C++ 的对比

| 特性 | C++ | C-Hat |
|------|-----|-------|
| 重载解析 | 复杂的排名系统 | 简单保守策略 |
| 隐式转换 | 允许链式转换 | 单级转换 |
| 歧义处理 | 可能产生意外行为 | 强制显式解决 |
| 类型安全 | 较弱 | 较强 |

## 未来可能的扩展

### 1. 用户定义的优先级

考虑允许用户为隐式转换指定优先级：

```cpp
extension LiteralView {
    // 高优先级转换
    [priority(high)]
    public implicit operator byte![]() { ... }
    
    // 低优先级转换
    [priority(low)]
    public implicit operator byte!^() { ... }
}
```

### 2. 上下文敏感选择

根据使用上下文选择最合适的转换：

```cpp
// 如果函数内部需要长度信息，优先选择 byte![]
// 如果函数内部只需要指针，优先选择 byte!^
```

## 示例代码

### 示例 1：基本重载

```cpp
import std.core.LiteralView;

func print(data: byte![]) -> void {
    // 使用切片版本，可以获取长度
    for (let i = 0; i < data.len; i++) {
        // 处理每个字节
    }
}

func print(data: byte!^) -> void {
    // 使用指针版本，假设以 null 结尾
    // 处理直到遇到 '\0'
}

func main() -> int {
    // 编译错误：歧义的函数调用
    // print("hello");
    
    // 正确：显式指定
    print("hello" as byte![]);
    print("hello" as byte!^);
    
    return 0;
}
```

### 示例 2：单一转换路径

```cpp
import std.core.LiteralView;

// 只有切片版本
func process(data: byte![]) -> void { }

func main() -> int {
    // 正确：唯一可行路径
    process("hello");
    return 0;
}
```

### 示例 3：精确匹配

```cpp
func foo(x: int) -> void { }
func foo(x: long) -> void { }

func main() -> int {
    foo(42);      // 选择 foo(int)
    foo(42L);     // 选择 foo(long)
    
    let i: int = 42;
    foo(i);       // 选择 foo(int) - 精确匹配
    
    return 0;
}
```

## 结论

C-Hat 的函数重载选择采用保守策略：**歧义时报错**。这确保了代码的清晰性和可预测性，避免了因复杂的隐式转换规则导致的意外行为。虽然这需要程序员在某些情况下显式指定类型转换，但这种显式性提高了代码的可读性和可维护性。

---

*最后更新：2026-03-05*
