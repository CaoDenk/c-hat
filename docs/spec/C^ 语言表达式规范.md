# C^ 语言表达式规范

本文档详细定义 C^ 语言的表达式语法、运算符优先级和结合性，以及各种表达式的用法。

## 1. 概述

C^ 语言提供了丰富的表达式类型和运算符，用于构建各种复杂的计算和逻辑操作。

## 2. 表达式类型

### 2.1 基本表达式

| 表达式类型     | 语法                 | 示例                           |
|-----------|--------------------|------------------------------|
| 字面量       | `literal`          | `42`, `3.14`, `"hello"`, `true` |
| 标识符       | `identifier`       | `x`, `y`, `result`            |
| 括号表达式    | `( expression )`   | `(a + b) * c`                |
| 数组访问      | `expression[expression]` | `arr[0]`, `matrix[i][j]`    |
| 成员访问      | `expression.member` | `obj.name`, `str.length`      |
| 指针成员访问   | `expression->member` | `ptr->x`, `ptr->y`          |
| 函数调用      | `expression(arguments)` | `add(1, 2)`, `println("hi")` |
| 指针解引用     | `expression^`      | `ptr^`                        |
| 取地址       | `^expression`      | `^x`                         |
| 移动语义      | `expression~`      | `obj~`                       |

### 2.2 复合表达式

| 表达式类型     | 语法                 | 示例                           |
|-----------|--------------------|------------------------------|
| 赋值表达式    | `lhs = rhs`        | `x = 5`, `a = b + c`          |
| 算术表达式    | `lhs op rhs`       | `a + b`, `x * y`, `i - 1`     |
| 比较表达式    | `lhs op rhs`       | `a == b`, `x < y`, `i >= 0`   |
| 逻辑表达式    | `lhs op rhs`       | `a && b`, `x || y`, `!flag`  |
| 位表达式      | `lhs op rhs`       | `a & b`, `x | y`, `i ^ j`     |
| 条件表达式    | `condition ? true_expr : false_expr` | `x > 0 ? x : -x` |
| 范围表达式    | `start..end`       | `0..9`, `i..j`                |
| 逗号表达式    | `expr1, expr2`     | `x = 1, y = 2`                |
| Lambda表达式  | `[...] (args) => expr` | `[](int a, int b) => a + b` |
| 箭头函数     | `(args) => expr`   | `(a, b) => a + b`             |

## 3. Lambda 表达式

C^ 的 Lambda 设计旨在结合 C++ 的高性能（零成本抽象）和现代脚本语言的简洁性。

### 3.1 核心语法

C^ 采用了 **C++ 风格** 的 `[]` 语法，因为它提供了最直观的捕获列表控制位置，且与函数调用语法天然契合。

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

### 3.2 捕获机制 (Captures)

C^ 的 Lambda 设计摒弃了隐式捕获的"魔法"，强制要求开发者明确捕获语义，以避免悬垂引用和意外拷贝。

#### 3.2.1 显式捕获 (Explicit Capture)

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

#### 3.2.2 移动捕获 (Move Capture)

C^ 支持移动捕获，用于转移所有权：

```cpp
var ptr = new int(5);
var owner = [x = ptr~] { delete x; };
```

### 3.3 类型系统

Lambda 在 C^ 中是 **匿名结构体 (Functor)**，但兼容标准库的 `Function` 接口。

#### 3.3.1 自动推导
通常配合 `var` 或 `let` 使用。

```cpp
var f = [](int x) => x * x;
```

#### 3.3.2 函数类型接口
当需要存储或传递 Lambda 时，使用 `func<Sig>` 类型。

```cpp
// 接受一个 int -> int 的函数
func apply(func<int(int)> f, int val) {
    return f(val);
}

apply([](int x) => x * 2, 10);
```

### 3.4 简写语法 (Shorthand)

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

### 3.5 递归 Lambda (Recursion)

C^ 的 Lambda 支持直接递归调用。为了解决 Lambda 在赋值前无法引用自身的问题，我们引入了 **`self` 参数**。

#### 3.5.1 显式 `self` 参数
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

### 3.6 Lambda 表达式总结

*   **语法**：回归经典的 `[] (...) => ...`。
*   **捕获**：**默认严格显式**（`[]` 不捕获）。支持 `[&]` (引用) 和 `[=]` (值) 以及 `[x, &y]` 混合模式。
*   **简写**：推荐箭头函数 `x => expr`（隐含按值捕获 `[=]`），移除隐式 `it`。
*   **递归**：通过 `self` 参数优雅实现递归。

## 4. 运算符

### 4.1 算术运算符

| 运算符 | 描述                 | 示例                           |
|-----|--------------------|------------------------------|
| `+`  | 加法                 | `a + b`                      |
| `-`  | 减法                 | `a - b`                      |
| `*`  | 乘法                 | `a * b`                      |
| `/`  | 除法                 | `a / b`                      |
| `%`  | 取模                 | `a % b`                      |
| `**` | 幂运算               | `2 ** 3` = 8                 |
| `++` | 自增（前缀/后缀）            | `++a`, `a++`                        |
| `--` | 自减（前缀/后缀）            | `--a`, `a--`                        |

### 4.2 比较运算符

| 运算符 | 描述                 | 示例                           |
|-----|--------------------|------------------------------|
| `==` | 等于                 | `a == b`                     |
| `!=` | 不等于                | `a != b`                     |
| `<`  | 小于                 | `a < b`                      |
| `>`  | 大于                 | `a > b`                      |
| `<=` | 小于等于               | `a <= b`                     |
| `>=` | 大于等于               | `a >= b`                     |
| `is` | 类型检查               | `x is int`                   |
| `as` | 类型转换               | `x as string`                |

### 4.3 逻辑运算符

| 运算符 | 描述                 | 示例                           |
|-----|--------------------|------------------------------|
| `!`  | 逻辑非                | `!a`                         |
| `&&` | 逻辑与                | `a && b`                     |
| `||` | 逻辑或                | `a || b`                     |

### 4.4 位运算符

| 运算符 | 描述                 | 示例                           |
|-----|--------------------|------------------------------|
| `~`  | 按位取反               | `~a`                         |
| `&`  | 按位与                | `a & b`                      |
| `^`  | 按位异或               | `a ^ b`                      |
| `|`  | 按位或                | `a | b`                      |
| `<<` | 左移                 | `a << b`                     |
| `>>` | 右移                 | `a >> b`                     |

### 4.5 赋值运算符

| 运算符 | 描述                 | 示例                           |
|-----|--------------------|------------------------------|
| `=`  | 简单赋值               | `a = b`                      |
| `+=` | 加赋值                | `a += b`                     |
| `-=` | 减赋值                | `a -= b`                     |
| `*=` | 乘赋值                | `a *= b`                     |
| `/=` | 除赋值                | `a /= b`                     |
| `%=` | 模赋值                | `a %= b`                     |
| `&=` | 按位与赋值              | `a &= b`                     |
| `|=` | 按位或赋值              | `a |= b`                     |
| `^=` | 按位异或赋值             | `a ^= b`                     |
| `<<=` | 左移赋值               | `a <<= b`                    |
| `>>=` | 右移赋值               | `a >>= b`                    |

### 4.6 其他运算符

| 运算符 | 描述                 | 示例                           |
|-----|--------------------|------------------------------|
| `sizeof` | 类型大小               | `sizeof(int)`, `sizeof(x)`    |
| `typeof` | 类型信息               | `typeof(int)`, `typeof(x)`    |
| `..`  | 范围运算符              | `0..9`                       |
| `,`  | 逗号运算符              | `x = 1, y = 2`                |
| `^`  | 取地址（前缀） / 解引用（后缀） | `^x`, `p^`                   |
| `->` | 指针成员访问            | `ptr->field`                 |
| `~`  | 移动语义（后缀）         | `obj~`                       |
| `$`  | 倒序索引（前缀）         | `arr[$1]`                    |

## 5. 运算符优先级与结合性

详细的运算符优先级和结合性请参考《C^ 语言运算符优先级规范》。

## 6. 赋值表达式

### 6.1 简单赋值

```cpp
int x = 5;
string name = "Alice";
bool flag = true;
```

### 6.2 复合赋值

```cpp
int x = 10;
x += 5; // 等价于 x = x + 5
x -= 2; // 等价于 x = x - 2
x *= 3; // 等价于 x = x * 3
x /= 2; // 等价于 x = x / 2
x %= 4; // 等价于 x = x % 4

// 位运算复合赋值
int y = 0b1010;
y &= 0b1100; // 等价于 y = y & 0b1100
y |= 0b0011; // 等价于 y = y | 0b0011
y ^= 0b1111; // 等价于 y = y ^ 0b1111
y <<= 2; // 等价于 y = y << 2
y >>= 1; // 等价于 y = y >> 1
```

### 6.3 多重赋值

```cpp
int a, b, c;
a = b = c = 10;

// 元组赋值
(int, string) person = ("Bob", 30);
string name;
int age;
(name, age) = person;

// 函数返回值赋值
func get_values() -> (int, int, int) {
    return (1, 2, 3);
}

int x, y, z;
(x, y, z) = get_values();
```

## 7. 算术表达式

### 7.1 基本算术运算

```cpp
int sum = 5 + 3;
int difference = 10 - 4;
int product = 6 * 7;
int quotient = 20 / 5;
double decimal_quotient = 20.0 / 3.0;
int remainder = 17 % 5;

// 幂运算
int power = 2 ** 3; // 8
```

### 7.2 自增和自减

```cpp
int x = 5;
int y = ++x; // x 变为 6，y 为 6

int a = 5;
int b = a++; // a 变为 6，b 为 5

int p = 10;
int q = --p; // p 变为 9，q 为 9

int m = 10;
int n = m--; // m 变为 9，n 为 10
```

## 8. 比较表达式

### 8.1 基本比较运算

```cpp
bool equal = (5 == 5);
bool not_equal = (5 != 3);
bool less = (3 < 5);
bool greater = (5 > 3);
bool less_or_equal = (3 <= 5);
bool greater_or_equal = (5 >= 3);
```

### 8.2 类型检查和转换

```cpp
// 类型检查
bool isInt = x is int;

// 类型转换
string str = x as string;
```

## 9. 逻辑表达式

### 9.1 逻辑运算

```cpp
bool flag = true;
bool not_flag = !flag;

bool result1 = (5 > 3) && (10 < 20);
bool result2 = (5 < 3) && (10 < 20);

bool result3 = (5 > 3) || (10 < 20);
bool result4 = (5 < 3) || (10 < 20);
```

## 10. 位表达式

### 10.1 位运算

```cpp
int a = 0b1010;
int not_a = ~a;

int b = 0b1100;
int and_result = a & b;
int or_result = a | b;
int xor_result = a ^ b;

int d = 0b1010;
int left_shift = d << 2;
int right_shift = d >> 2;
```

## 11. 条件表达式

### 11.1 三元运算符

```cpp
int x = 10;
int max = (x > y) ? x : y;

int a = 10;
int b = 20;
int c = 15;
int largest = (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c);

string result = (x > 0) ? "Positive" : "Non-positive";
```

## 12. 范围表达式

### 12.1 范围运算

```cpp
int[] numbers = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];

// 数组切片
int[] slice1 = numbers[0..5]; // [0, 1, 2, 3, 4]
int[] slice2 = numbers[2..8]; // [2, 3, 4, 5, 6, 7]

// 范围用于循环
for (int i in 0..9) {
    println(i);
}

// 范围用于 match 表达式
int value = 5;
string category = match (value) {
    0..9 => "Single digit",
    10..99 => "Double digit",
    _ => "Large number"
};
```

## 13. 表达式语句

表达式可以作为语句使用，通常用于函数调用、赋值操作等：

```cpp
x = 5;
add(1, 2);
counter++;
ptr^ = 100;
```

## 14. 表达式求值顺序

### 14.1 一般规则

- 表达式的求值顺序通常是从左到右
- 但在某些情况下，求值顺序可能会受到运算符优先级和结合性的影响
- 函数参数的求值顺序是从左到右

### 14.2 副作用

当表达式包含副作用（如修改变量值、I/O 操作等）时，需要注意求值顺序：

```cpp
int x = 0;
int result = x++ + x++; // 结果可能因编译器而异，应避免此类代码

int y = 0;
func f(int a, int b) -> int { return a + b; }
int func_result = f(y++, y++); // 参数按从左到右求值，结果为 0 + 1 = 1
```

## 15. 常量表达式

C^ 支持常量表达式，即在编译期求值的表达式：

```cpp
const int MAX_SIZE = 100;
const int ARRAY_SIZE = MAX_SIZE / 2;
const bool DEBUG = true;

const int FACTORIAL_5 = 5 * 4 * 3 * 2 * 1;

int[ARRAY_SIZE] buffer;

const int VALUE = DEBUG ? 1 : 0;
```

## 16. 总结

C^ 语言提供了丰富的表达式类型和运算符，支持各种复杂的计算和逻辑操作。本文档详细描述了：

- 各种表达式类型的语法和用法
- 运算符的优先级和结合性
- 各种运算符的具体操作和示例
- 表达式的求值顺序和副作用
- 常量表达式的使用
- Lambda 表达式的完整语法和特性

通过合理使用这些表达式和运算符，可以编写清晰、高效的 C^ 代码。在使用过程中，应注意运算符优先级和结合性，以及表达式的副作用，以避免潜在的错误。
