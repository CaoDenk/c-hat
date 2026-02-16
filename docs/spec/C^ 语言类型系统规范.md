# C^ 语言类型系统规范

## 1. 概述

C^ 语言采用静态类型系统，所有变量和表达式的类型在编译期确定。类型系统设计旨在提供足够的类型安全保障，同时保持高性能和灵活性。

## 2. 基础类型

C^ 提供了丰富的基础数据类型，满足不同场景的需求。

### 2.1 整数类型

| 类型     | 大小  | 取值范围                       | 描述           |
| -------- | ----- | ------------------------------ | -------------- |
| `byte`   | 1字节 | 0 ~ 255                        | 无符号8位整数  |
| `sbyte`  | 1字节 | -128 ~ 127                     | 有符号8位整数  |
| `short`  | 2字节 | -32,768 ~ 32,767               | 有符号16位整数 |
| `ushort` | 2字节 | 0 ~ 65,535                     | 无符号16位整数 |
| `int`    | 4字节 | -2,147,483,648 ~ 2,147,483,647 | 有符号32位整数 |
| `uint`   | 4字节 | 0 ~ 4,294,967,295              | 无符号32位整数 |
| `long`   | 8字节 | -9e18 ~ 9e18                   | 有符号64位整数 |
| `ulong`  | 8字节 | 0 ~ 18e18                      | 无符号64位整数 |

### 2.2 浮点类型

| 类型     | 大小  | 精度         | 描述                     |
| -------- | ----- | ------------ | ------------------------ |
| `float`  | 4字节 | 单精度浮点数 | 32位IEEE 754浮点数       |
| `double` | 8字节 | 双精度浮点数 | 64位IEEE 754浮点数       |
| `fp16`   | 2字节 | 半精度浮点数 | 16位IEEE 754浮点数       |
| `bf16`   | 2字节 | 脑浮点格式   | 16位Brain Floating Point |

### 2.3 字符类型

| 类型   | 大小  | 取值范围     | 描述                    |
| ------ | ----- | ------------ | ----------------------- |
| `char` | 4字节 | 0 ~ 0x10FFFF | Unicode字符，UTF-32码点 |

### 2.4 布尔类型

| 类型   | 大小  | 取值范围          | 描述   |
| ------ | ----- | ----------------- | ------ |
| `bool` | 1字节 | `true` 或 `false` | 布尔值 |

**注意**：
- C^ 采用**严格布尔类型**。
- 不允许 `int`、指针等类型隐式转换为 `bool`。
- 条件表达式（`if`, `while` 等）必须使用 `bool` 类型。
- 用户自定义类型可通过 `explicit operator bool` 实现布尔上下文转换。

## 3. 指针类型

C^ 提供了安全的指针类型，通过语法区分非空指针和可空指针。

### 3.1 指针语法

| 类型语法          | 描述                                      |
| ----------------- | ----------------------------------------- |
| `Type^`           | 非空指针，指向 `Type` 类型的对象          |
| `Optional<Type^>` | 可空指针，指向 `Type` 类型的对象 (标准库) |
| `Type&`           | 可变左值引用，必须绑定到左值              |
| `Type!&`          | 不可变左值引用，可绑定左值或右值          |
| `Type~`           | 右值引用，必须绑定到右值                  |
| `Type&~`          | 万能引用，可绑定左值或右值                |

### 3.2 引用绑定规则

C^ 对引用的绑定有严格的规则，以确保内存安全和语义清晰。

| 引用类型                 | 目标类别      | 调用语法                          | 说明                                                   |
| :----------------------- | :------------ | :-------------------------------- | :----------------------------------------------------- |
| **可变左值引用** `T&`    | 左值 (Lvalue) | `foo(&x)`                         | 必须显式使用 `&` 取址，表明该值可能被修改。            |
| **不可变左值引用** `T!&` | 左值/右值     | `foo(x)` / `foo(10)`              | 隐式绑定。绑定右值时会延长临时对象生命周期。           |
| **右值引用** `T~`        | 右值 (Rvalue) | `foo(x~)` / `foo(10)`             | 显式使用 `~` 将左值转为右值（Move）。右值隐式绑定。    |
| **万能引用** `T&~`       | 左值/右值     | `foo(&x)` / `foo(x~)` / `foo(10)` | 仅用于泛型参数。根据实参类别推导类型，并进行引用折叠。 |

> **注意**：
> - `T!&` 类似于 C++ 的 `const T&`，是传递大对象的默认方式。
> - `T~` 类似于 C++ 的 `T&&` (非推导上下文)，用于实现移动语义。
> - `T&~` 是 C^ 特有的万能引用语法，仅在泛型推导上下文中有效。它依赖于编译器根据实参的值类别来推导 `T`，再由类型系统进行引用折叠。对于非泛型类型（如 `int&~`），由于不存在类型推导过程，无法区分左值和右值，因此是非法的。对于具体类型，请根据需求选择 `int!&`（只读，通用）或分别重载 `int&` 和 `int~`。
> - `&` 只用于把“普通左值”显式变成可变引用实参；如果表达式本身已经是引用类型（例如 `T&` / `T~` / `T&~` 折叠后的参数），再次传递时不需要重复写 `&`。

### 3.2.1 引用折叠（用于 `T&~`）

`T&~` 的折叠规则定义为：

- 若 `T` 被推导为 `U&`，则 `T&~` 折叠为 `U&`
- 否则 `T` 为非引用类型 `U`，则 `T&~` 折叠为 `U~`

这使得标准库可以实现无开洞的完美转发：

```cpp
// 推荐直接使用显式转发强转，而不是依赖 std.forward：
// ((T&~)x)
```

### 3.3 指针操作

| 操作符 | 描述         | 示例                        |
| ------ | ------------ | --------------------------- |
| `^`    | 取地址操作符 | `int a = 5; int^ ptr = ^a;` |
| `^`    | 解引用操作符 | `int value = ptr^;`         |

### 3.3 指针使用示例

```cpp
// 非空指针
int value = 42;
int^ ptr = ^value; // 取地址
ptr^ = 100;        // 解引用并修改值

// 可空指针 (使用 Optional<T>)
Optional<int^> nullable_ptr = null;
if (nullable_ptr) {
    int value = nullable_ptr.value()^; // 安全解引用
}

// 指针数组
int^[5] ptr_array;
for (int i = 0; i < 5; i++) {
    int^[i] = ^(int)i; // 注意：这里只是示例，实际需要确保内存有效性
}
```

## 4. 数组类型

C^ 提供了固定大小的数组和动态大小的切片。

### 4.1 数组语法

| 类型语法  | 描述                                           |
| --------- | ---------------------------------------------- |
| `Type[N]` | 固定大小数组，包含 N 个 `Type` 类型的元素      |
| `Type[$]` | 栈数组推导，大小由初始化器推导，栈上分配       |
| `Type[]`  | 动态大小切片，引用一段连续内存中的元素         |
| `Type![]` | 不可变切片，引用一段连续内存中的元素，不可修改 |

### 4.2 数组操作

#### 4.2.1 固定大小数组

```cpp
// 声明固定大小数组
int[5] arr = {1, 2, 3, 4, 5};

// 访问数组元素
int first = arr[0];
int last = arr[4];

// 修改数组元素
arr[2] = 10;

// 数组长度
int length = sizeof(arr) / sizeof(arr[0]);
```

#### 4.2.2 栈数组推导

```cpp
// 栈数组推导 - 大小由初始化器自动推导
int[$] arr1 = [1, 2, 3]; // 等价于 int[3]
int[$] arr2 = [1, 2, 3, 4, 5]; // 等价于 int[5]

// 栈数组推导的优势：简洁且零开销
// 编译器在编译期确定大小，分配在栈上
```

#### 4.2.3 切片

```cpp
// 从固定数组创建切片
int[5] arr = {1, 2, 3, 4, 5};
int[] slice = arr[1..3]; // 包含元素 2, 3

// 切片长度
int length = slice.length;

// 访问切片元素
int first = slice[0];

// 修改切片元素（会影响原数组）
slice[1] = 20;
```

#### 4.2.4 不可变切片

不可变切片 `Type![]` 是 C^ 提供的一种安全的切片类型，确保数据不被意外修改。

**不可变切片特性**：
- `Type![]` 是 `Type[]` 的子类型
- 不能通过下标修改元素
- 可以传递给期望 `Type[]` 的函数（协变）

**字符串字面量**：
- 字符串字面量 `"hello"` 的默认类型是 `std.literal_view`
- `std.literal_view` 可以隐式转换为 `byte![]`（不可变字节切片）
- `std.literal_view` 保证指向的数据拥有静态生命周期

```cpp
// 不可变切片 - 不能修改切片内容
int[5] arr = {1, 2, 3, 4, 5};
int![] immutable_slice = arr[0..3]; // 不可变切片

// 可以读取元素
int first = immutable_slice[0]; // OK

// 不能修改元素
// immutable_slice[1] = 20; // 编译错误：不可修改不可变切片

// 不可变切片可以传递给可变切片参数（协变）
func print_ints(int[] arr) { ... }
print_ints(immutable_slice); // OK：int![] 可隐式转换为 int[]

// 字符串字面量默认为 std.literal_view
var s = "hello"; // 类型为 std.literal_view
// s[0] = 'H'; // 编译错误：不能修改字符串字面量

// 字符串字面量可以传递给不可变切片
byte![] bytes = "hello"; // std.literal_view -> byte![]

// 强制栈分配（可变）
byte[$] s_stack = "hello"; // 在栈上分配，可修改
s_stack[0] = 'H'; // OK
```

### 4.3 多维数组

```cpp
// 二维固定大小数组
int[3][4] matrix = {
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12}
};

// 访问二维数组元素
int value = matrix[1][2]; // 7

// 二维切片
int[][] matrix_slice = matrix[0..2][1..3];
```

## 5. 字符串类型

C^ 提供了两种字符串类型：`string` 和 `string_view`。

### 5.1 字符串语法

| 类型          | 描述                                 |
| ------------- | ------------------------------------ |
| `string`      | 不可变字符串，UTF-8编码              |
| `string_view` | 字符串视图，不拥有内存，仅引用字符串 |

### 5.2 字符串字面量

```cpp
// 字符串字面量默认为 std.literal_view
var s = "hello"; // 类型为 std.literal_view

// 可以隐式转换为 string
string hello = "Hello, World!"; // std.literal_view 隐式转换为 string

// 字符串插值
int age = 30;
string message = "My age is age}";

// 原始字符串
string path = r"C:\\Users\\Name\\Documents";

// 多行字符串
string poem = """
Roses are red,
Violets are blue,
Sugar is sweet,
And so are you.
""";

// 字符串字面量不可修改
var literal = "hello";
// literal[0] = 'H'; // 编译错误：不能修改 std.literal_view
```

### 5.3 字符串操作

```cpp
// 字符串连接
string greeting = "Hello" + " " + "World!";

// 字符串长度
int length = greeting.length;

// 字符串索引
char first_char = greeting[0]; // 'H'

// 字符串切片
string sub_str = greeting[0..5]; // "Hello"

// 字符串比较
bool equal = greeting == "Hello World!";

// 字符串查找
int index = greeting.find("World");
```

## 6. 元组类型

C^ 支持元组类型，用于表示多个值的组合。

### 6.1 元组语法

```cpp
// 元组类型声明
(string, int) person;

// 元组初始化
person = ("Alice", 30);

// 元组解构
(string name, int age) = person;

// 元组元素访问
string person_name = person.0;
int person_age = person.1;

// 元组作为返回值
func get_employee() -> (string, int) {
    return ("Bob", 25);
}

// 使用元组返回值
(string employee_name, int employee_age) = get_employee();
```

### 6.2 元组作为返回值

```cpp
func get_person() -> (string, int) {
    return ("Charlie", 35);
}

// 使用元组返回值
(string name, int age) = get_person();
```

## 7. 函数类型

C^ 支持函数类型，用于表示函数的签名。

### 7.1 函数类型语法

```cpp
// 函数类型
func(int, string) -> bool

// 函数类型变量
func(int, string) -> bool predicate;

// 函数类型赋值
predicate = func(int x, string s) -> bool {
    return x > 0 && s.length > 0;
};

// 函数类型作为参数
func process(func(int, string) -> bool pred, int value, string text) {
    if (pred(value, text)) {
        // 处理逻辑
    }
}

// 函数类型作为返回值
func create_predicate(bool strict) -> func(int, string) -> bool {
    if (strict) {
        return func(int x, string s) -> bool { return x > 0 && s.length > 0; };
    } else {
        return func(int x, string s) -> bool { return x >= 0 || s.length > 0; };
    }
}
```

## 8. 类型别名

C^ 支持类型别名，用于为复杂类型提供更简洁的名称。

### 8.1 类型别名语法

```cpp
// 类型别名
using IntPtr = int^;
using StringMap = Map<string, string>;
using Callback = func(int, string) -> void;

// 使用类型别名
IntPtr ptr = ^(int)42;
StringMap config;
Callback on_complete;
```

## 9. 类型转换

C^ 支持显式类型转换和隐式类型转换。

### 9.1 隐式类型转换

隐式类型转换在类型安全的情况下自动进行，例如：

```cpp
// 小整数类型转换为大整数类型
byte b = 42;
int i = b; // 隐式转换

// 整数转换为浮点数
int x = 100;
double d = x; // 隐式转换

// 派生类指针转换为基类指针
class Base {}
class Derived : Base {}
Derived^ d = new Derived();
Base^ b = d; // 隐式转换
```

### 9.2 显式类型转换

显式类型转换需要使用转换操作符，例如：

```cpp
// 大整数类型转换为小整数类型
int i = 1000;
byte b = (byte)i; // 显式转换，可能丢失数据

// 浮点数转换为整数
float f = 3.14;
int x = (int)f; // 显式转换，截断小数部分

// 指针类型转换
void^ ptr = ^(int)42;
int^ int_ptr = (int^)ptr; // 显式转换
```

### 9.3 安全类型转换

C^ 提供了安全的类型转换操作符 `as`，用于安全地转换引用类型：

```cpp
// 安全类型转换
class Base {}
class Derived : Base {}

Base^ b = new Derived();
Derived^ d = b as Derived; // 安全转换，如果失败返回 null

if (d != null) {
    // 转换成功
} else {
    // 转换失败
}
```

## 10. 类型推断

C^ 支持类型推断，使用 `var` 关键字：

```cpp
// 类型推断
var x = 42; // 推断为 int 类型
var y = 3.14; // 推断为 double 类型
var s = "Hello"; // 推断为 string 类型
var list = [1, 2, 3]; // 推断为 int[] 类型

// 类型推断与函数返回值
var result = get_person(); // 推断为 (string, int) 类型

// 类型推断与泛型
var map = Map<string, int>(); // 推断为 Map<string, int> 类型
```

## 11. 类型约束

C^ 支持泛型类型约束，用于限制泛型参数的类型范围。

### 11.1 类型约束语法

```cpp
// 类型约束
func<T>(T value) where T : IComparable -> bool {
    // 实现
}

// 多个类型约束
func<T>(T value) where T : IComparable, IDisposable -> bool {
    // 实现
}

// 泛型类的类型约束
class Container<T> where T : new() {
    T create() {
        return new T();
    }
}
```

## 12. 类型安全

C^ 设计了多种机制来确保类型安全：

### 12.1 数组边界检查

```cpp
int[5] arr = {1, 2, 3, 4, 5};
arr[10] = 100; // 编译时错误：索引越界

int[] slice = arr[0..5];
slice[10] = 200; // 运行时错误：索引越界
```

### 12.2 空指针检查

```cpp
Optional<int^> ptr = null;
// ptr.value()^ = 5; // 运行时错误：空值解包

// 安全检查
if (ptr.has_value()) {
    ptr.value()^ = 5; // 安全操作
}
```

### 12.3 类型转换检查

```cpp
class Base {}
class Derived : Base {}
class Other {}

Base^ b = new Derived();
Other^ o = (Other^)b; // 运行时错误：类型转换失败

// 安全转换
Other^ safe_o = b as Other; // 返回 null，不抛出异常
```

## 13. 总结

C^ 语言的类型系统设计兼顾了类型安全、性能和表达能力：

- **丰富的基础类型**：满足不同场景的需求
- **安全的指针类型**：区分非空指针和可空指针
- **灵活的数组和切片**：支持固定大小和动态大小的序列
- **现代化的字符串处理**：支持字符串插值、原始字符串等特性
- **强大的元组类型**：方便地组合多个值
- **函数类型**：支持一等函数
- **类型推断**：减少冗余代码
- **类型约束**：增强泛型的安全性
- **类型安全机制**：数组边界检查、空指针检查等

C^ 的类型系统设计旨在提供足够的类型安全保障，同时保持与 C/C++ 类似的高性能和灵活性，为系统编程提供理想的类型基础。
