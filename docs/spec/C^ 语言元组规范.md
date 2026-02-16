# C^ 语言元组规范

## 1. 概述

元组 (Tuple) 是 C^ 语言中的一种内置数据结构，用于将多个不同类型的值组合成一个单一的类型。元组是值类型，在栈上分配，提供零开销的类型安全数据聚合。

### 1.1 元组核心特性

- **值类型**：元组在栈上分配，没有堆分配开销，性能与手写结构体完全一致
- **类型安全**：每个元素都有明确的类型，编译期类型检查
- **匿名数据结构**：无需预先定义结构体类型即可创建数据聚合
- **解构支持**：支持模式匹配解构，轻松处理多返回值
- **命名与无名**：支持命名元组和无名元组两种形式

### 1.2 元组的优势

- **简化代码**：避免为简单数据聚合定义临时结构体
- **多返回值**：函数可以返回多个值而无需定义额外的类型
- **临时数据**：适合存储临时组合的数据
- **模式匹配**：与解构语法配合，提供优雅的数据处理方式

## 2. 元组基础

### 2.1 元组构造

元组使用圆括号 `()` 定义，元素之间用逗号 `,` 分隔。

#### 语法

```
tuple_expression ::= '(' expression_list ')'
expression_list ::= expression (',' expression)*
tuple_type ::= '(' type_list ')'
type_list ::= type (',' type)*
```

#### 示例

```cpp
// 类型推导元组
var t1 = (1, "hello", 3.14);           // 类型为 (int, string, double)
var t2 = (10, 20);                      // 类型为 (int, int)
var t3 = ("Alice", 25, true);           // 类型为 (string, int, bool)

// 显式类型元组
(int, int) point = (10, 20);
(string, int, bool) user = ("Bob", 30, false);
```

### 2.2 元组类型

元组类型使用圆括号括起来的类型列表表示。

#### 语法

```
tuple_type ::= '(' type_list ')'
type_list ::= type (',' type)*
```

#### 示例

```cpp
// 元组类型
(int, int)           // 两个整数的元组
(string, int)        // 字符串和整数的元组
(int, string, double) // 三个不同类型的元组
```

### 2.3 元组元素数量

元组至少包含 2 个元素。C^ 不支持单元素元组。

#### 示例

```cpp
// 合法
var t1 = (1, 2);           // 2个元素
var t2 = (1, 2, 3);        // 3个元素

// 不支持单元素元组
// var t3 = (1,);          // 编译错误：不支持单元素元组
```

## 3. 命名元组

命名元组允许为元组的每个元素指定名称，提供更好的可读性。

### 3.1 命名元组构造

#### 语法

```
named_tuple_type ::= '(' named_element_list ')'
named_element_list ::= named_element (',' named_element)*
named_element ::= type identifier
```

#### 示例

```cpp
// 命名元组类型
(int x, int y) point = (10, 20);
(string name, int age) person = ("Alice", 25);
(int width, int height, string title) window = (1920, 1080, "Main Window");
```

### 3.2 命名元组访问

命名元组支持通过字段名和下标两种方式访问元素。

#### 示例

```cpp
(string name, int age) user = ("Bob", 30);

// 通过字段名访问
print(user.name);  // "Bob"
print(user.age);   // 30

// 通过下标访问
print(user[0]);    // "Bob"
print(user[1]);    // 30
```

## 4. 元组成员访问

### 4.1 下标访问

无名元组使用编译期常量下标 `[N]` 访问元素。

#### 语法

```
tuple_subscript ::= tuple_expression '[' integer_literal ']'
```

#### 示例

```cpp
var t = (100, 200, 300);
print(t[0]); // 100
print(t[1]); // 200
print(t[2]); // 300
```

#### 限制

- 下标必须是编译期常量
- 下标必须在有效范围内 `[0, N-1]`，其中 N 是元组元素数量

### 4.2 字段名访问

命名元组可以通过字段名访问元素。

#### 示例

```cpp
(int x, int y) p = (10, 20);
print(p.x); // 10
print(p.y); // 20
```

### 4.3 混合访问

命名元组同时支持字段名和下标访问。

#### 示例

```cpp
(string name, int age, bool active) user = ("Charlie", 35, true);

print(user.name);   // "Charlie"
print(user[0]);     // "Charlie"
print(user.age);    // 35
print(user[1]);     // 35
print(user.active); // true
print(user[2]);     // true
```

## 5. 元组解构

解构是将元组拆解为独立变量的语法糖。

### 5.1 声明并解构

在声明变量的同时解构元组。

#### 语法

```
destructuring_declaration ::= var_pattern '=' tuple_expression
var_pattern ::= 'var'? '(' pattern_list ')'
pattern_list ::= pattern (',' pattern)*
pattern ::= identifier | '_'
```

#### 示例

```cpp
func get_point() -> (int, int) {
    return (10, 20);
}

func main() {
    var (x, y) = get_point();
    print(x); // 10
    print(y); // 20
}
```

### 5.2 赋值解构

将元组赋值给已存在的变量。

#### 示例

```cpp
func get_rect() -> (int, int, int, int) {
    return (0, 0, 1920, 1080);
}

func main() {
    int x, y, width, height;
    (x, y, width, height) = get_rect();
    
    print(x);      // 0
    print(y);      // 0
    print(width);  // 1920
    print(height); // 1080
}
```

### 5.3 忽略值

使用下划线 `_` 忽略不需要的值。

#### 示例

```cpp
func get_user_info() -> (string, int, string, string) {
    return ("Alice", 25, "alice@example.com", "1234567890");
}

func main() {
    var (name, age, _, _) = get_user_info();
    print(name); // "Alice"
    print(age);  // 25
    // email 和 phone 被忽略
}
```

### 5.4 交换变量

使用元组解构实现变量交换。

#### 示例

```cpp
func main() {
    int a = 10;
    int b = 20;
    
    print(a); // 10
    print(b); // 20
    
    (a, b) = (b, a);
    
    print(a); // 20
    print(b); // 10
}
```

### 5.5 嵌套解构

支持嵌套元组的解构。

#### 示例

```cpp
func get_nested() -> ((int, int), (string, int)) {
    return ((10, 20), ("Alice", 25));
}

func main() {
    var ((x, y), (name, age)) = get_nested();
    print(x);    // 10
    print(y);    // 20
    print(name); // "Alice"
    print(age);  // 25
}
```

## 6. 元组与函数

### 6.1 元组作为返回类型

函数可以返回元组，实现多返回值。

#### 示例

```cpp
func divide(int a, int b) -> (int, int) {
    return (a / b, a % b);
}

func main() {
    var (quotient, remainder) = divide(17, 5);
    print(quotient);  // 3
    print(remainder); // 2
}
```

### 6.2 元组作为参数

函数可以接受元组作为参数。

#### 示例

```cpp
func print_point((int, int) point) {
    print(point[0]);
    print(point[1]);
}

func main() {
    var p = (10, 20);
    print_point(p);
}
```

### 6.3 元组展开

在函数调用时展开元组作为参数。

#### 示例

```cpp
func add(int a, int b, int c) -> int {
    return a + b + c;
}

func main() {
    var args = (1, 2, 3);
    var result = add(args...); // 展开元组
    print(result); // 6
}
```

## 7. 元组比较

### 7.1 相等性比较

元组支持相等性比较 `==` 和 `!=`。

#### 规则

- 两个元组相等当且仅当它们的元素数量相同，且对应位置的元素都相等
- 比较按字典序进行

#### 示例

```cpp
func main() {
    var t1 = (1, 2, 3);
    var t2 = (1, 2, 3);
    var t3 = (1, 2, 4);
    
    print(t1 == t2); // true
    print(t1 == t3); // false
    print(t1 != t3); // true
}
```

### 7.2 关系比较

元组支持关系比较 `<`, `<=`, `>`, `>=`。

#### 规则

- 比较按字典序进行
- 首先比较第一个元素，如果相等则比较第二个元素，以此类推

#### 示例

```cpp
func main() {
    var t1 = (1, 2);
    var t2 = (1, 3);
    var t3 = (2, 1);
    
    print(t1 < t2); // true (2 < 3)
    print(t1 < t3); // true (1 < 2)
    print(t2 < t3); // true (1 < 2)
}
```

## 8. 元组与逗号运算符

### 8.1 逗号运算符的移除

C^ 移除了 C 语言风格的逗号运算符。在 C^ 中，`(a, b)` 永远表示构造一个元组，而不是"执行 a 丢弃结果再执行 b"。

#### C 语言 vs C^

```cpp
// C 语言
int x = (a, b); // 执行 a，丢弃结果，返回 b

// C^ (不支持)
// int x = (a, b); // 编译错误：不能将元组转换为 int

// C^ 正确用法
var t = (a, b); // t 是一个元组
```

### 8.2 for 循环中的逗号

`for` 循环的 `init` 和 `update` 子句显式允许逗号分隔的表达式列表。

#### 示例

```cpp
func main() {
    for (int i = 0, j = 10; i < j; i++, j--) {
        print(i + j);
    }
}
```

## 9. 歧义消解

### 9.1 分组 vs 元组

- `(expr)`：被视为表达式分组（括号）
- `(expr1, expr2)`：被视为元组

#### 示例

```cpp
var a = (10);           // a 是 int，括号用于分组
var b = (10, 20);       // b 是 (int, int) 元组
var c = ((10, 20));     // c 是 (int, int) 元组，外层括号用于分组
```

### 9.2 函数调用中的元组

函数调用中的元组需要特别注意。

#### 示例

```cpp
func foo(int a, int b) {
    print(a + b);
}

func main() {
    var t = (1, 2);
    
    // 错误：不能直接传递元组
    // foo(t); // 编译错误
    
    // 正确：展开元组
    foo(t...);
    
    // 正确：直接传递
    foo(1, 2);
}
```

## 10. 内存布局与 ABI

### 10.1 内存布局

元组在内存中遵循 C 结构体的对齐规则。

- `(int, int)` 布局等同于 `struct { int f0; int f1; }`
- 支持按值传递和按引用传递
- 元素按声明顺序在内存中连续存储

#### 示例

```cpp
// 内存布局
(int, int) t1 = (10, 20);
// 等价于 struct { int f0; int f1; } t1 = {10, 20};

(string, int, bool) t2 = ("hello", 42, true);
// 等价于 struct { string f0; int f1; bool f2; } t2 = {"hello", 42, true};
```

### 10.2 传递方式

#### 按值传递

```cpp
func process((int, int) point) {
    print(point[0]);
    print(point[1]);
}

func main() {
    var p = (10, 20);
    process(p); // 按值传递，复制整个元组
}
```

#### 按引用传递

```cpp
func process((int, int)!& point) {
    point[0] = 100;
    point[1] = 200;
}

func main() {
    var p = (10, 20);
    process(p); // 按引用传递，修改原元组
    print(p[0]); // 100
    print(p[1]); // 200
}
```

## 11. 元组与泛型

### 11.1 泛型元组

元组可以与泛型结合使用。

#### 示例

```cpp
func first<T1, T2>((T1, T2) tuple) -> T1 {
    return tuple[0];
}

func second<T1, T2>((T1, T2) tuple) -> T2 {
    return tuple[1];
}

func main() {
    var t1 = (10, "hello");
    print(first(t1));  // 10
    print(second(t1)); // "hello"
    
    var t2 = (3.14, 42);
    print(first(t2));  // 3.14
    print(second(t2)); // 42
}
```

### 11.2 元组类型推导

泛型函数可以推导元组类型。

#### 示例

```cpp
func print_tuple<T...>(T... tuple) {
    comptime for (i in 0...tuple.length - 1) {
        print(tuple[i]);
    }
}

func main() {
    print_tuple(1, "hello", 3.14);
    // 输出：
    // 1
    // hello
    // 3.14
}
```

## 12. 元组与模式匹配

### 12.1 match 语句中的元组

元组可以在 `match` 语句中进行模式匹配。

#### 示例

```cpp
func describe((int, int) point) -> string {
    match point {
        (0, 0) => "Origin",
        (x, 0) => "On x-axis",
        (0, y) => "On y-axis",
        (x, y) => "General point"
    }
}

func main() {
    print(describe((0, 0)));    // "Origin"
    print(describe((10, 0)));   // "On x-axis"
    print(describe((0, 20)));   // "On y-axis"
    print(describe((10, 20)));  // "General point"
}
```

### 12.2 嵌套模式匹配

支持嵌套元组的模式匹配。

#### 示例

```cpp
func process(((int, int), (string, int)) data) -> string {
    match data {
        ((0, 0), (name, _)) => name + " at origin",
        ((x, y), (name, age)) => name + " at (" + x + ", " + y + ")"
    }
}

func main() {
    var data = ((10, 20), ("Alice", 25));
    print(process(data)); // "Alice at (10, 20)"
}
```

## 13. 元组与标准库

### 13.1 常用元组操作

标准库提供了一些常用的元组操作函数。

#### 示例

```cpp
// 假设标准库提供以下函数
func make_tuple<T...>(T... values) -> (T...) {
    return (T...);
}

func tuple_size<T...>((T...) tuple) -> int {
    return sizeof...(T);
}

func tuple_element<I, T...>((T...) tuple) -> T_I {
    return tuple[I];
}
```

### 13.2 元组算法

标准库提供了一些元组算法。

#### 示例

```cpp
func tuple_for_each<T..., F>((T...) tuple, F func) {
    comptime for (i in 0...sizeof...(T) - 1) {
        func(tuple[i]);
    }
}

func main() {
    var t = (1, "hello", 3.14);
    tuple_for_each(t, (value) => print(value));
    // 输出：
    // 1
    // hello
    // 3.14
}
```

## 14. 最佳实践

### 14.1 使用元组的场景

1. **多返回值**：函数需要返回多个相关值时
2. **临时数据聚合**：需要临时组合多个值时
3. **解构赋值**：需要同时处理多个值时
4. **数据交换**：需要交换变量值时

### 14.2 避免使用元组的场景

1. **复杂数据结构**：数据结构复杂或有语义时，使用命名结构体
2. **频繁访问**：频繁访问特定字段时，使用命名结构体提高可读性
3. **公共 API**：公共 API 应使用命名结构体提供更好的文档和可维护性

### 14.3 命名元组 vs 无名元组

- **命名元组**：适合有明确语义的数据，提高代码可读性
- **无名元组**：适合临时数据或语义不明确的场景

#### 示例

```cpp
// 推荐：命名元组用于有语义的数据
(string name, int age) get_user() {
    return ("Alice", 25);
}

var (name, age) = get_user();
print(name); // 清晰

// 推荐：无名元组用于临时数据
var (min, max) = find_min_max(array);
```

## 15. 与其他语言的对比

| 特性           | C++ std::tuple    | Rust Tuple | Python Tuple | C^ Tuple |
| -------------- | ----------------- | ---------- | ------------ | -------- |
| 语法           | `std::make_tuple` | `(a, b)`   | `(a, b)`     | `(a, b)` |
| 访问           | `std::get<0>(t)`  | `t.0`      | `t[0]`       | `t[0]`   |
| 命名支持       | ❌                 | ❌          | ❌            | ✅        |
| 解构           | `std::tie`        | ✅          | ✅            | ✅        |
| 值类型         | ✅                 | ✅          | ❌            | ✅        |
| 零开销         | ✅                 | ✅          | ❌            | ✅        |
| 单元素元组     | ❌                 | ❌          | ✅            | ❌        |
| 逗号运算符冲突 | ✅                 | ❌          | ❌            | ❌        |

## 16. 总结

C^ 的元组提供了强大而简洁的数据聚合能力：

- **简洁语法**：`(a, b)` 构造元组，`t[0]` 访问元素
- **类型安全**：编译期类型检查，零运行时开销
- **解构支持**：优雅的多返回值和数据交换
- **命名支持**：命名元组提高代码可读性
- **零开销**：栈上分配，性能与手写结构体完全一致
- **消除歧义**：移除逗号运算符，`(a, b)` 永远表示元组

通过合理使用元组，可以编写更简洁、更安全的 C^ 代码。
