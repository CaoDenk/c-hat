# C^ Lambda 表达式设计

C^ 的 Lambda 设计旨在结合 C++ 的高性能（零成本抽象）和现代脚本语言的简洁性。

## 1. 核心语法

C^ 采用了 **C++ 风格** 的 `[]` 语法，因为它提供了最直观的捕获列表控制位置，且与函数调用语法天然契合。

### 1.1 基础语法：`[...] (args) => expr`

*   **`[]`**：捕获列表（必须存在，作为 Lambda 的标志）。
*   **`(args)`**：参数列表（无参数时可省略）。
*   **`=>`**：单表达式箭头（简洁）或 `{ ... }`（多行）。

```cpp
// 1. 单表达式 Lambda (自动推导返回值)
var add = [](int a, int b) => a + b;

// 2. 多语句 Lambda (带花括号)
var max = [](int a, int b) {
    if (a > b) return a;
    return b;
};

// 3. 无参数 Lambda (省略括号)
var say_hello = [] => print("Hello");
```

## 2. 捕获机制 (Captures)

C^ 的 Lambda 设计摒弃了隐式捕获的"魔法"，强制要求开发者明确捕获语义，以避免悬垂引用和意外拷贝。

### 2.1 显式捕获 (Explicit Capture)

当 `[]` 为空时，Lambda **不捕获任何外部变量**（无状态）。如果代码中使用了外部局部变量，会导致编译错误。

*   **`[]` (Empty)**：不捕获。仅能访问全局变量、静态成员或参数。
*   **`[&]` (Reference)**：**按引用捕获**所有使用的外部变量。
    *   高效，无拷贝开销。
    *   **注意**：必须确保 Lambda 的生命周期短于被捕获变量（通常用于立即执行的回调，如 `sort`, `filter`）。如果 Lambda 逃逸（被返回或存储），会导致悬垂引用。
*   **`[=]` (Value)**：**按值捕获**所有使用的外部变量（只读副本）。
    *   安全，Lambda 拥有数据副本，可以安全逃逸。
    *   注意：对于大对象可能会有拷贝开销。
*   **`[x, &y]` (Mixed)**：混合指定。`x` 按值，`y` 按引用。

```cpp
int x = 10;
int y = 20;

// 1. 错误：[] 不捕获，无法访问 x, y
// var sum = [] => x + y; // Error

// 2. 引用捕获：适合作为回调立即使用
var sum_ref = [&] => x + y; 

// 3. 值捕获：适合返回或异步执行
var sum_val = [=] => x + y;
```

### 2.2 移动捕获 (Move Capture)
var ptr = new int(5);
var owner = [x = ptr~] { delete x; };
```

## 3. 类型系统

Lambda 在 C^ 中是 **匿名结构体 (Functor)**，但兼容标准库的 `Function` 接口。

### 3.1 自动推导
通常配合 `var` 或 `let` 使用。

```cpp
var f = [](int x) => x * x;
```

### 3.2 函数类型接口
当需要存储或传递 Lambda 时，使用 `func<Sig>` 类型。

```cpp
// 接受一个 int -> int 的函数
func apply(func<int(int)> f, int val) {
    return f(val);
}

apply([](int x) => x * 2, 10);
```

## 4. 简写语法 (Shorthand)

对于简单的单参数 Lambda，C^ 支持**箭头函数**语法，省略 `[]` 和类型。

```cpp
var numbers = [1, 2, 3, 4];

// 1. 完整写法
numbers.filter([](int n) => n > 2);

// 2. 箭头函数 (Arrow Function) - 推荐
// 类似 C# / JS，参数类型自动推导
// 语义：默认采用 [=] (按值捕获)，以保证安全性和不可变性。
// 如果需要引用捕获 (如修改外部变量)，请使用完整语法 `[&](...) { ... }`。
numbers.filter(n => n > 2);

// 3. 极简写法 (Implicit 'it') - 已移除
// 早期设计曾考虑省略参数名直接用 it，但为了避免歧义和魔法变量，
// 我们决定移除 `filter(it > 2)` 这种语法。
// 必须显式声明参数： `numbers.filter(it => it > 2)`。
```

## 5. 递归 Lambda (Recursion)

C^ 的 Lambda 支持直接递归调用。为了解决 Lambda 在赋值前无法引用自身的问题，我们引入了 **`self` 参数**。

### 5.1 显式 `self` 参数
通过在参数列表中显式声明 `self`（类型为 Lambda 自身类型），可以在函数体内安全地进行递归调用。

```cpp
// 斐波那契数列 (递归 Lambda)
var fib = [](self, int n) -> int {
    if (n <= 1) return n;
    return self(n - 1) + self(n - 2);
};

print(fib(10)); // 输出 55
```

*   **机制**：`self` 实际上是对当前闭包对象的引用。
*   **优势**：避免了 C++ 中需要使用 `std::function` 或 Y-Combinator 才能实现 Lambda 递归的尴尬。

## 6. 总结

*   **语法**：回归经典的 `[] (...) => ...`。
*   **捕获**：**默认严格显式**（`[]` 不捕获）。支持 `[&]` (引用) 和 `[=]` (值) 以及 `[x, &y]` 混合模式。
*   **简写**：推荐箭头函数 `x => expr`（隐含按值捕获 `[=]`），移除隐式 `it`。
*   **递归**：通过 `self` 参数优雅实现递归。
