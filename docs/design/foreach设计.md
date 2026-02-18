# C^ foreach 设计文档

## 1. 概述

C^ 的 `foreach` 语句是为了简化容器遍历而设计的语法糖，提供了统一、简洁、安全的方式来遍历数组、切片、元组、字符串以及标准库容器。

## 2. 基本语法

### 2.1 基础形式

```cpp
foreach (Type element in container) {
    // 使用 element
}
```

### 2.2 类型推导形式

```cpp
foreach (var element in container) {
    // 编译器自动推导 element 类型
}
```

### 2.3 索引访问形式（可选）

```cpp
foreach (var index, var element in container) {
    // index 是当前元素的索引，从 0 开始
    // element 是当前元素
}
```

## 3. 支持的容器类型

### 3.1 数组与切片

```cpp
int[5] arr = [1, 2, 3, 4, 5];
int[] slice = [10, 20, 30];

// 遍历固定数组
foreach (var x in arr) {
    println(x);
}

// 遍历切片
foreach (var x in slice) {
    println(x);
}

// 带索引遍历
foreach (var i, var x in arr) {
    println("[{}] = {}", i, x);
}
```

### 3.2 字符串

```cpp
var str = "Hello, C^!";

// 遍历字符串中的字符
foreach (var ch in str) {
    println(ch);
}
```

### 3.3 元组

```cpp
var t = (100, "hello", 3.14);

// 遍历元组元素
foreach (var x in t) {
    println(x);
}
```

### 3.4 标准库容器

```cpp
import std.collections.List;

var list = List<int> { 1, 2, 3, 4, 5 };

// 遍历 List
foreach (var x in list) {
    println(x);
}
```

## 4. 可变性与引用

### 4.1 默认：值拷贝（不可变）

```cpp
int[] arr = [1, 2, 3];

foreach (var x in arr) {
    x = 100; // Error: x 是只读的值拷贝
}
```

### 4.2 可变引用

```cpp
int[] arr = [1, 2, 3];

// 使用 & 获取可变引用
foreach (var &x in arr) {
    x = 100; // OK: 修改原数组元素
}

// 结果: arr = [100, 100, 100]
```

### 4.3 不可变引用（只读视图）

```cpp
int[] arr = [1, 2, 3];

// 使用 !& 获取不可变引用
foreach (let &x in arr) {
    println(x); // OK: 只读访问
    // x = 100; // Error: 不可修改
}
```

## 5. 范围 for 循环

C^ 也支持 C++ 风格的范围语法 `for (in)`，作为 `foreach` 的替代语法：

```cpp
// 两种写法等价
foreach (var x in arr) { ... }
for (var x in arr) { ... }
```

**设计决策：** 提供两种语法是为了兼容开发者的不同背景，但推荐使用 `foreach` 以提高可读性。

## 6. 实现机制

### 6.1 底层展开

`foreach` 语句在编译器内部会展开为传统的 `for` 循环。

对于数组和切片：

```cpp
// 原始代码
foreach (var x in arr) {
    println(x);
}

// 编译器展开为
for (var i = 0; i < arr.length; i++) {
    var x = arr[i];
    println(x);
}
```

对于可变引用版本：

```cpp
// 原始代码
foreach (var &x in arr) {
    x = 100;
}

// 编译器展开为
for (var i = 0; i < arr.length; i++) {
    var &x = arr[i];
    x = 100;
}
```

### 6.2 迭代器协议

对于用户自定义类型，`foreach` 通过迭代器协议工作。一个类型只要实现以下方法，就能被 `foreach` 遍历：

```cpp
class MyContainer<T> {
    // 返回迭代器起始位置
    public func begin() -> Iterator<T>;
    
    // 返回迭代器结束位置
    public func end() -> Iterator<T>;
}

class Iterator<T> {
    // 获取当前元素（解引用）
    public func operator*() -> T&;
    
    // 移动到下一个元素（前缀递增）
    public func operator++() -> Iterator<T>&;
    
    // 比较两个迭代器是否相等
    public func operator==(Iterator<T> other) -> bool;
    
    // 比较两个迭代器是否不相等
    public func operator!=(Iterator<T> other) -> bool;
}
```

### 6.3 编译器优化

编译器会对 `foreach` 进行以下优化：

1. **边界检查消除**：对于固定大小数组，编译期已知长度，可以消除运行时边界检查
2. **循环展开**：对于小循环，编译器可能会完全展开循环
3. **向量化**：对于连续内存布局的容器，编译器可能生成 SIMD 指令

## 7. 设计对比

| 特性       | C++ (range-based for) | C# (foreach)           | C^ (foreach)               |
| ---------- | --------------------- | ---------------------- | -------------------------- |
| 语法       | `for (auto x : c)`    | `foreach (var x in c)` | `foreach (var x in c)`     |
| 默认行为   | 值拷贝/引用取决于类型 | 只读访问               | 只读值拷贝                 |
| 可变访问   | 使用引用类型          | 需要特殊语法           | 使用 `&`                   |
| 索引访问   | 不支持                | 不直接支持             | 支持 `foreach (i, x in c)` |
| 迭代器协议 | 基于 begin/end        | 基于 IEnumerable       | 基于 begin/end 或 长度索引 |

## 8. 最佳实践

### 8.1 何时使用 foreach

- ✅ 遍历数组、切片、字符串等序列类型
- ✅ 不需要手动管理索引
- ✅ 代码简洁性优先

### 8.2 何时使用传统 for 循环

- ✅ 需要复杂的索引操作
- ✅ 需要非顺序遍历（反向、跳跃等）
- ✅ 需要在循环中修改索引

### 8.3 性能考虑

- 对于只读访问，默认值拷贝通常足够高效
- 对于大型对象或需要修改元素的场景，使用引用版本 `&`
- 对于固定大小数组，`foreach` 的性能与手动 `for` 循环相同

## 9. 示例

### 9.1 求和

```cpp
func sum(int[] arr) -> int {
    int s = 0;
    foreach (var x in arr) {
        s += x;
    }
    return s;
}
```

### 9.2 查找元素

```cpp
func find(int[] arr, int target) -> int? {
    foreach (var i, var x in arr) {
        if (x == target) {
            return i;
        }
    }
    return null;
}
```

### 9.3 修改元素

```cpp
func double(int[] arr) {
    foreach (var &x in arr) {
        x *= 2;
    }
}
```

## 10. 总结

C^ 的 `foreach` 设计提供了：

1. **简洁性**：语法清晰，减少样板代码
2. **安全性**：默认只读，避免意外修改
3. **灵活性**：支持值拷贝、可变引用和不可变引用
4. **性能**：编译器优化保证与手动循环相同的效率
5. **通用性**：支持数组、切片、字符串、元组和自定义容器

`foreach` 是 C^ 中推荐的遍历方式，适用于大多数场景。
