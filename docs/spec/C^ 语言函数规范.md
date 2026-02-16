# C^ 语言函数规范

## 1. 概述

C^ 语言中的函数是代码组织的基本单位，用于封装可重用的逻辑。本文档详细描述 C^ 语言中函数的声明、定义、参数传递、返回值、重载、泛型等特性。

## 2. 函数声明与定义

### 2.1 函数声明

函数声明用于告诉编译器函数的存在及其签名，不包含函数体：

```cpp
// 函数声明
func add(int, int) -> int;
func print_message(string) -> void;
func get_person() -> (string, int);
```

### 2.2 函数定义

函数定义包含函数声明和函数体：

```cpp
// 函数定义
func add(int a, int b) -> int {
    return a + b;
}

func print_message(string message) -> void {
    println(message);
}

func get_person() -> (string, int) {
    return ("Alice", 30);
}
```

### 2.3 函数签名

函数签名由以下部分组成：
- 函数名
- 参数类型列表
- 返回类型

函数签名用于区分不同的函数，函数重载基于不同的签名。

### 2.4 异常说明 (Exception Specification)

使用 `noexcept` 关键字标记函数不会抛出异常。这是函数签名的一部分，有助于编译器优化。

```cpp
// 保证不抛出异常
func swap(int& a, int& b) -> void noexcept {
    int temp = a;
    a = b;
    b = temp;
}

// 条件 noexcept
func process() -> void noexcept(is_safe()) {
    // ...
}
```

## 3. 函数参数

### 3.1 参数声明

参数声明的语法为：`Type name`，其中 `Type` 是参数类型，`name` 是参数名：

```cpp
func func_name(Type1 param1, Type2 param2, ...) -> ReturnType {
    // 函数体
}
```

### 3.2 参数传递

C^ 语言中的参数传递方式：

1. **值传递**：默认的参数传递方式，传递参数的副本
2. **可变引用传递**：使用 `&` 修饰符，传递参数的引用。**调用时必须显式使用 `&`**，让副作用在调用点可见
3. **不可变引用传递**：使用 `!&` 修饰符，传递参数的常量引用。**调用时可以直接传递变量**（因为无副作用）
4. **右值引用传递**：使用 `~` 修饰符，传递参数的所有权
5. **万能引用**：使用 `T&~` 修饰符，用于泛型中的完美转发

```cpp
// 值传递
func increment(int x) -> int {
    x++;
    return x;
}

// 可变引用传递 - 调用时必须显式使用 &
func increment_ref(int& x) -> void {
    x++;
}

// 不可变引用传递 - 调用时可以直接传递变量
func print_value(int!& x) -> void {
    println(x);
}

// 右值引用传递
func take_ownership(string~ s) -> void {
    println(s);
    // s 的所有权已转移，函数结束后会被销毁
}

// 万能引用 (泛型)
func<T> forward_wrapper(T&~ arg) -> void {
    target((T&~)arg);
}

// 调用示例
func main() -> void {
    int val = 10;
    
    increment(val);       // 值传递
    increment_ref(&val);  // 可变引用传递 - 显式使用 &
    print_value(val);     // 不可变引用传递 - 直接传递变量
    
    take_ownership(string("temp")); // 临时对象自动匹配右值引用
    string s = "hello";
    take_ownership(s~);   // 显式 move 传递
}
```

完美转发场景是例外：当参数类型为 `T&~`（仅泛型推导上下文）时，推荐用 `((T&~)x)` 作为显式“转发标记”，由编译器按 `&~` 规则选择绑定到 `T&` 或 `T~`，从而避免手写 `?:`。

### 3.2.1 设计哲学与 C++ 对比

C^ 的引用传递设计经过深思熟虑，旨在解决 C++ 中长期存在的**可见性**和**安全性**问题。

| 特性              | C++ 做法            | C^ 设计     | C^ 的优势                                                                                                                              |
| :---------------- | :------------------ | :---------- | :------------------------------------------------------------------------------------------------------------------------------------- |
| **可变引用调用**  | `foo(x)`            | `foo(&x)`   | **拒绝隐式副作用**。在 C++ 中，仅看 `foo(x)` 无法判断 `x` 是否会被修改。C^ 强制使用 `&`，让代码审查者一眼看出副作用。                  |
| **右值/移动调用** | `foo(std::move(x))` | `foo(x~)`   | **简洁且语义统一**。`~` 在 C^ 中代表"析构/生命周期结束"。`x~` 语义上表示"x 的生命周期在此结束，资源被转移"，比冗长的库函数调用更优雅。 |
| **只读引用调用**  | `foo(x)`            | `foo(x)`    | **安全默认**。对于不会修改参数的 `int!&`，保持调用的简洁性（因为无副作用）。                                                           |
| **引用符号**      | `T&` / `T&&`        | `T&` / `T~` | **符号区分度高**。`&` 代表引用，`~` 代表移动/右值。避免了 C++ 中 `&` 和 `&&` 容易混淆的问题。                                          |
```

### 3.3 默认参数

C^ 支持为函数参数提供默认值：

```cpp
// 默认参数
func greet(string name, string message = "Hello") -> string {
    return message + ", " + name + "!";
}

// 调用带默认参数的函数
string greeting1 = greet("Alice"); // 使用默认值 "Hello"
string greeting2 = greet("Bob", "Hi"); // 使用指定值 "Hi"
```

### 3.4 可变参数

C^ 支持可变参数函数，使用 `...` 语法：

```cpp
// 可变参数函数
func sum(int count, ...) -> int {
    int result = 0;
    // 处理可变参数
    return result;
}

// 调用可变参数函数
int total = sum(3, 1, 2, 3); // 6
```

## 4. 返回值

### 4.1 基本返回值

函数可以返回各种类型的值：

```cpp
// 返回基本类型
func get_answer() -> int {
    return 42;
}

// 返回字符串
func get_greeting() -> string {
    return "Hello, World!";
}

// 返回布尔值
func is_positive(int x) -> bool {
    return x > 0;
}
```

### 4.2 多返回值

C^ 支持使用元组返回多个值：

```cpp
// 多返回值
func divide(int a, int b) -> (int, bool) {
    if (b == 0) {
        return (0, false);
    }
    return (a / b, true);
}

// 调用多返回值函数
(int result, bool success) = divide(10, 2);
if (success) {
    println("Result: " + result);
} else {
    println("Division by zero");
}
```

### 4.3 无返回值

使用 `void` 表示函数无返回值：

```cpp
// 无返回值函数
func print_hello() -> void {
    println("Hello!");
}

// 调用无返回值函数
print_hello();
```

### 4.4 单表达式函数

对于简单的函数，可以使用箭头语法 `=>` 定义单表达式函数：

```cpp
// 单表达式函数
func add(int a, int b) -> int => a + b;
func is_even(int x) -> bool => x % 2 == 0;
func get_default() -> string => "Default";
```

## 5. 函数重载

C^ 支持函数重载，即多个函数可以有相同的名称但不同的参数列表：

```cpp
// 函数重载
func add(int a, int b) -> int => a + b;
func add(double a, double b) -> double => a + b;
func add(string a, string b) -> string => a + b;

// 调用重载函数
int int_sum = add(1, 2); // 调用第一个 add
 double double_sum = add(1.5, 2.5); // 调用第二个 add
string string_sum = add("Hello", " World"); // 调用第三个 add
```

**重载规则**：
- 函数名必须相同
- 参数列表必须不同（参数类型、数量或顺序）
- 返回类型不同不足以区分重载

## 6. 泛型函数

C^ 支持泛型函数，使用类型参数来实现通用算法：

### 6.1 基本泛型函数

```cpp
// 泛型函数
func<T> print_value(T value) -> void {
    println(value);
}

// 调用泛型函数
print_value<int>(42);
print_value<string>("Hello");
print_value<double>(3.14);

// 类型推断
print_value(42); // 推断为 int
print_value("Hello"); // 推断为 string
print_value(3.14); // 推断为 double
```

### 6.2 泛型约束

使用 `where` 子句为泛型参数添加约束，基于 Concept 机制：

```cpp
// 带约束的泛型函数
func<T> maximum(T a, T b) -> T where Comparable<T> {
    return a > b ? a : b;
}

// 多个约束
func<T> process(T value) -> void where Disposable<T> && Cloneable<T> {
    // 处理逻辑
}

// 约束为具体类型 (使用 SameType 概念)
func<T> convert_to_string(T value) -> string where SameType<T, int> || SameType<T, double> {
    return value.ToString();
}
```

### 6.3 多类型参数

```cpp
// 多类型参数泛型函数
func<T, U> pair(T first, U second) -> (T, U) {
    return (first, second);
}

// 调用多类型参数泛型函数
(string, int) person = pair<string, int>("Alice", 30);
(int, double) values = pair(10, 3.14); // 类型推断
```

### 6.4 万能引用与完美转发

结合泛型，C^ 支持万能引用 `T&~` 和完美转发 `forward<T>`，用于编写高效的包装函数：

```cpp
// 万能引用：T &~ 可以绑定左值也可以绑定右值
func<T> wrapper(T &~ arg) -> void {
    // forward<T> 保持参数的值类别（左值或右值）
    process(forward<T>(arg));
}

// 完美转发构造函数
class Widget {
    string name;
    int id;
    
    // 接受任意参数并转发给成员变量的构造函数
    public Widget<U, V>(U &~ name_arg, V &~ id_arg) {
        self.name = forward<U>(name_arg);
        self.id = forward<V>(id_arg);
    }
}
```

## 7. 函数指针与委托

### 7.1 函数类型

C^ 支持函数类型，可以将函数作为参数传递或作为返回值：

```cpp
// 函数类型
func(int, int) -> int

// 函数类型变量
func(int, int) -> int operation;

// 函数类型赋值
operation = add;
int result = operation(1, 2); // 3

// 函数类型作为参数
func apply_operation(int a, int b, func(int, int) -> int op) -> int {
    return op(a, b);
}

// 调用带函数参数的函数
int sum = apply_operation(5, 3, add);
int product = apply_operation(5, 3, func(int a, int b) -> int => a * b);

// 函数类型作为返回值
func get_operation(string op_name) -> func(int, int) -> int {
    if (op_name == "add") {
        return add;
    } else if (op_name == "multiply") {
        return func(int a, int b) -> int => a * b;
    } else {
        return func(int a, int b) -> int => 0;
    }
}
```

### 7.2 Lambda 表达式

Lambda 表达式是一种创建匿名函数的简洁方式，C^ 采用了 C++ 风格的 `[]` 语法：

#### 7.2.1 基本语法

```cpp
// 单表达式 Lambda (自动推导返回值)
var add = [](int a, int b) => a + b;

// 多语句 Lambda (带花括号)
var max = [](int a, int b) {
    if (a > b) return a;
    return b;
};

// 无参数 Lambda (省略括号)
var say_hello = [] => print("Hello");
```

#### 7.2.2 捕获机制 (Captures)

C^ 的 Lambda 设计摒弃了隐式捕获的"魔法"，强制要求开发者明确捕获语义：

*   **`[]` (Empty)**：不捕获。仅能访问全局变量、静态成员或参数。
*   **`[&]` (Reference)**：**按引用捕获**所有使用的外部变量。
    *   高效，无拷贝开销。
    *   **注意**：必须确保 Lambda 的生命周期短于被捕获变量（通常用于立即执行的回调）。
*   **`[=]` (Value)**：**按值捕获**所有使用的外部变量（只读副本）。
    *   安全，Lambda 拥有数据副本，可以安全逃逸。
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

// 4. 混合捕获
var mixed = [x, &y] => x + y;
```

#### 7.2.3 移动捕获

C^ 支持移动捕获，用于转移所有权：

```cpp
var ptr = new int(5);
var owner = [x = ptr~] { delete x; };
```

#### 7.2.4 简写语法 (Shorthand)

对于简单的单参数 Lambda，C^ 支持**箭头函数**语法，省略 `[]` 和类型：

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

#### 7.2.5 递归 Lambda (Recursion)

C^ 的 Lambda 支持直接递归调用。为了解决 Lambda 在赋值前无法引用自身的问题，我们引入了 **`self` 参数**：

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

#### 7.2.6 Lambda 作为参数

```cpp
// Lambda 作为参数
func filter<T>(T[] array, func(T) -> bool predicate) -> T[] {
    // 实现过滤逻辑
}

int[] numbers = [1, 2, 3, 4, 5];
int[] even_numbers = filter(numbers, [](int x) => x % 2 == 0);

// 使用箭头函数
int[] odd_numbers = filter(numbers, x => x % 2 != 0);
```

#### 7.2.7 Lambda 总结

*   **语法**：回归经典的 `[] (...) => ...`。
*   **捕获**：**默认严格显式**（`[]` 不捕获）。支持 `[&]` (引用) 和 `[=]` (值) 以及 `[x, &y]` 混合模式。
*   **简写**：推荐箭头函数 `x => expr`（隐含按值捕获 `[=]`），移除隐式 `it`。
*   **递归**：通过 `self` 参数优雅实现递归。

## 8. 递归函数

C^ 支持递归函数，即函数可以调用自身：

```cpp
// 递归函数
func factorial(int n) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 调用递归函数
int result = factorial(5); // 120

// 尾递归优化
func fibonacci(int n, int a = 0, int b = 1) -> int {
    if (n == 0) {
        return a;
    }
    return fibonacci(n - 1, b, a + b);
}
```

**注意**：递归函数可能导致栈溢出，对于深层递归，应考虑使用迭代或尾递归优化。

## 9. 内联函数

使用 `inline` 关键字建议编译器将函数内联展开，减少函数调用开销：

```cpp
// 内联函数
inline func add(int a, int b) -> int {
    return a + b;
}

// 内联单表达式函数
inline func multiply(int a, int b) -> int => a * b;
```

**注意**：`inline` 只是建议，编译器会根据具体情况决定是否内联。

## 10. comptime 函数

使用 `comptime` 关键字强制在编译期执行函数，返回编译期常量：

```cpp
// comptime 函数
comptime func factorial(int n) -> int {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

// 编译期计算
const int result = factorial(5); // 结果为 120

// comptime 函数用于元编程
comptime func is_power_of_two(int n) -> bool {
    return n > 0 && (n & (n - 1)) == 0;
}

// 编译期条件
const int SIZE = 64;
comptime func assert_power_of_two() -> void {
    static_assert(is_power_of_two(SIZE), "SIZE must be power of 2");
}
```

**适用场景**：
- 元编程
- 编译期计算
- 类型系统约束
- 编译期断言

**限制**：
- comptime 函数只能使用编译期可用的操作
- 不能调用非 comptime 函数
- 不能使用运行时内存分配

## 11. 静态函数

使用 `static` 关键字声明静态函数，静态函数只在当前文件内可见：

```cpp
// 静态函数
static func helper() -> void {
    // 实现辅助逻辑
}

// 静态函数只能在当前文件内调用
helper();
```

## 11. 成员函数

类的成员函数是定义在类内部的函数，用于操作类的实例：

```cpp
class Person {
    string name;
    int age;

    // 成员函数
    func greet() -> string {
        return "Hello, my name is " + name;
    }

    // 带参数的成员函数
    func set_age(int new_age) -> void {
        age = new_age;
    }

    // 带返回值的成员函数
    func get_age() -> int {
        return age;
    }
}

// 调用成员函数
Person^ p = new Person();
p.name = "Alice";
string greeting = p.greet();
p.set_age(30);
int person_age = p.get_age();
```

### 11.1 `self` 参数

C^ 中的成员函数可以显式声明 `self` 参数，用于访问实例：

```cpp
class Person {
    string name;

    // 显式 self 参数（可变引用）
    func set_name(&self, string new_name) -> void {
        self.name = new_name;
    }

    // 显式 self 参数（常量引用）
    func get_name(!self) -> string {
        return self.name;
    }

    // 显式 self 参数（移动）
    func take_ownership(~self) -> void {
        // 处理 self 的所有权
    }
}
```

## 12. 构造函数与析构函数

### 12.1 构造函数

构造函数用于初始化类的实例：

```cpp
class Person {
    string name;
    int age;

    // 构造函数
    public Person(string name, int age) {
        self.name = name;
        self.age = age;
    }

    // 带默认参数的构造函数
    public Person() {
        self.name = "Unknown";
        self.age = 0;
    }
}

// 调用构造函数
Person^ p1 = new Person("Alice", 30);
Person^ p2 = new Person();
```

### 12.2 析构函数

析构函数用于清理类的实例：

```cpp
class Resource {
    void^ handle;

    public Resource() {
        handle = allocate_resource();
    }

    // 析构函数
    public ~ {
        if (handle != null) {
            free_resource(handle);
            handle = null;
        }
    }
}

// 析构函数会在对象销毁时自动调用
Resource^ r = new Resource();
// 使用 r
// 离开作用域时，r 的析构函数会被调用
```

## 13. 函数指针与回调

### 13.1 函数指针

C^ 使用 `^` 后缀获取函数地址，函数指针类型语法为 `return_type (^pointer_name)(param_types)`：

```cpp
// 函数
func add(int a, int b) -> int {
    return a + b;
}

func subtract(int a, int b) -> int {
    return a - b;
}

// 函数指针变量
int (^operation)(int, int);

// 使用后缀 ^ 获取函数地址
operation = add^;
printf("%d\n", operation(5, 3)); // 输出 8

operation = subtract^;
printf("%d\n", operation(5, 3)); // 输出 2

// 函数指针类型别名
using Operation = int (^)(int, int);
Operation op = add^;
printf("%d\n", op(10, 20)); // 输出 30
```

### 13.2 回调函数

```cpp
// 回调函数类型
using Callback = func(int) -> void;

// 接受回调的函数
func process_data(int[] data, Callback callback) -> void {
    for (int i = 0; i < data.length; i++) {
        // 处理数据
        callback(data[i]);
    }
}

// 回调函数
func print_data(int value) -> void {
    println(value);
}

// 调用接受回调的函数
int[] numbers = [1, 2, 3, 4, 5];
process_data(numbers, print_data);

// 使用 Lambda 作为回调
process_data(numbers, (int value) => println("Value: " + value));
```

## 14. 函数作用域

函数内部声明的变量具有函数作用域，只在函数内部可见：

```cpp
func example() -> void {
    int x = 10; // 函数作用域变量
    
    if (x > 5) {
        int y = 20; // 块作用域变量，只在 if 块内可见
        println(x + y);
    }
    
    // 错误：y 在此处不可见
    // println(y);
    
    println(x); // 正确：x 在函数作用域内可见
}
```

## 15. 函数文档

使用文档注释为函数添加文档：

```cpp
/**
 * Adds two integers.
 * 
 * @param a The first integer.
 * @param b The second integer.
 * @return The sum of the two integers.
 */
func add(int a, int b) -> int {
    return a + b;
}

/**
 * Computes the factorial of a non-negative integer.
 * 
 * @param n A non-negative integer.
 * @return The factorial of n.
 * @throws ArgumentException If n is negative.
 */
func factorial(int n) -> int {
    if (n < 0) {
        throw ArgumentException("n must be non-negative");
    }
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

## 16. 性能考虑

### 16.1 函数调用开销

- 函数调用会产生开销，包括参数传递、栈帧创建、返回地址保存等
- 对于频繁调用的小函数，考虑使用 `inline` 关键字
- 对于深层递归，考虑使用迭代或尾递归优化

### 16.2 参数传递优化

- 对于大型对象，使用引用传递（`&`）或常量引用传递（`!`）避免拷贝
- 对于临时对象，使用移动传递（`~`）避免拷贝
- 合理使用默认参数和重载，提高代码可读性

### 16.3 内存管理

- 函数内部的局部变量在栈上分配，自动管理
- 函数内部使用 `new` 分配的内存需要手动 `delete`，或使用智能指针
- 注意避免内存泄漏和悬垂指针

## 17. 最佳实践

1. **函数命名**：使用清晰、描述性的函数名，遵循驼峰命名法
2. **函数大小**：函数应该保持小而专注，通常不超过 50-100 行
3. **函数职责**：每个函数应该只做一件事，并做好
4. **参数数量**：参数数量应该合理，通常不超过 3-5 个
5. **返回值**：函数应该有明确的返回值，避免使用输出参数
6. **错误处理**：使用异常或错误码处理错误，保持函数接口清晰
7. **文档**：为公共函数添加文档注释，说明函数的用途、参数和返回值
8. **测试**：为函数编写单元测试，确保功能正确

## 18. 总结

C^ 语言的函数系统提供了丰富的特性，包括：

- **灵活的参数传递**：支持值传递、引用传递、常量引用传递和移动传递
- **多返回值**：使用元组返回多个值
- **函数重载**：基于不同的参数列表重载函数
- **泛型函数**：使用类型参数实现通用算法
- **Lambda 表达式**：创建匿名函数
- **递归函数**：实现递归算法
- **内联函数**：减少函数调用开销
- **静态函数**：限制函数作用域
- **成员函数**：操作类的实例
- **构造函数和析构函数**：初始化和清理对象

通过合理使用这些特性，可以编写清晰、高效、可维护的 C^ 代码。
