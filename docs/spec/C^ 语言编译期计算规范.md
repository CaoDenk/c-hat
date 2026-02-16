# C^ 语言编译期计算规范

本文档详细定义 C^ 语言的编译期计算机制，包括 `comptime` 关键字、常量表达式、类型计算和元编程支持。

## 1. 概述

C^ 语言支持强大的编译期计算能力，允许在编译时执行代码，生成优化后的程序。主要特性包括：

- **常量表达式**：编译期可计算的表达式
- **`comptime` 关键字**：强制编译期执行
- **类型计算**：编译期类型操作
- **元编程**：代码生成和反射

## 2. 常量表达式

### 2.1 定义

常量表达式是在编译期可以完全计算的表达式，其结果在运行时不可改变。

### 2.2 `const` 关键字

如果你需要一个编译期常量（等价于 C++ 的 `static constexpr`），**必须**使用 `const` 关键字。`const` 定义的值是严格不可变的。

**注意：局部 `const` 并不隐含 `static` 存储期。**
在 C^ 中，`const` 仅代表**值**在编译期已知。如果 `const` 变量在函数内部定义，它仅仅是一个命名的编译期字面量，**不**具备静态存储期（除非显式加上 `static`）。这解决了 C++ 中 `static constexpr` 导致函数内常量隐含静态存储的问题。

```cpp
// 编译期常量 (Immutable)
public const PI = 3.14159;
public const MAX_USERS = 100;

func foo() {
    // 这是一个命名的字面量，不占用静态存储区
    // 编译器会将其内联到使用处
    const int PORT = 80; 
    
    // 如果需要静态存储期 (单例模式/跨调用持久化)，必须显式加上 static
    // static const int CACHED_PORT = 80;
}
```

### 2.3 常量表达式规则

以下情况构成常量表达式：

1. **字面量**：整数、浮点、字符、字符串、布尔、`null`
2. **常量变量**：`const` 声明的变量
3. **常量函数**：标记为 `comptime` 的纯函数
4. **运算符组合**：常量表达式的运算符组合
5. **条件表达式**：所有分支都是常量表达式

### 2.4 示例

```cpp
// 常量变量
const int SIZE = 100;
const double PI = 3.14159;

// 常量表达式
const int AREA = SIZE * SIZE;  // 编译期计算
const bool FLAG = true && false;

// 数组大小必须是常量
int arr[SIZE];  // OK
// int arr2[getSize()];  // 错误：非常量

// switch case 必须是常量
match (x) {
    SIZE => ...,  // OK
    // getValue() => ...,  // 错误
}
```

## 3. comptime 关键字

### 3.1 设计理念

`comptime` 关键字用于标记代码块、表达式或控制流语句在编译期求值。`comptime` 并不定义"常量"，而是定义"编译期执行的作用域"。

### 3.2 语法

```
comptime_declaration ::= 'comptime' declaration
comptime_expression ::= 'comptime' expression
comptime_block ::= 'comptime' compound_statement
comptime_if ::= 'comptime' 'if' '(' expression ')' statement ('else' statement)?
comptime_for ::= 'comptime' 'for' '(' ... ')' statement
```

### 3.3 语义

- `comptime` 强制在编译期执行
- 编译期执行的代码不能依赖于运行时的值
- 编译期代码可以访问编译器 API

### 3.4 `comptime` 块与局部变量

`comptime` 块内的变量仅在编译期存在。为了支持循环和复杂逻辑，这些**局部变量**是可变的。

```cpp
// 使用 comptime 块计算常量
public const FACT_5 = comptime {
    var result = 1; // 这是一个编译期局部变量，必须是可变的 (var)
    for (int i = 1; i <= 5; i++) {
        result *= i; // 允许修改编译期局部变量
    }
    return result; // 返回计算结果
};

// FACT_5 在运行时是不可变的常量 (120)
```

### 3.5 `comptime` 修饰变量 (不推荐)

虽然允许 `comptime var x = ...`，但这容易产生混淆。我们**强烈推荐**将 `comptime` 逻辑封装在代码块中，并将结果赋值给 `const`。

```cpp
// 不推荐的写法：容易混淆 x 是常量还是变量
// comptime var x = 10; 

// 推荐写法：明确 x 是一个常量，其值由编译期逻辑生成
const int x = comptime { 
    return 10 * 20; 
};
```

```cpp
// 编译期代码块
comptime {
    // 这段代码在编译期执行
    var types = [int, float, double];
    for (var T in types) {
        generateSerializer<T>();
    }
}
```

## 4. 编译期类型系统

### 4.1 类型操作

```cpp
// 获取类型信息
comptime var int_size = sizeof(int);
comptime var int_align = alignof(int);

// 类型特征
comptime var is_int = is_integral<int>;
comptime var is_ptr = is_pointer<int^>;

// 类型转换
comptime var ptr_type = add_pointer<int>;      // int^
comptime var ref_type = add_reference<int>;    // int&
comptime var base_type = remove_pointer<int^>; // int
```

### 4.2 类型列表

```cpp
// 编译期类型列表
comptime var numeric_types = [byte, short, int, long, float, double];

// 遍历类型列表
comptime {
    for (var T in numeric_types) {
        generateMathFunctions<T>();
    }
}
```

## 5. 编译期控制流

### 5.1 `comptime if` (Static If)

相当于 C++ 的 `if constexpr` 或预处理器的 `#ifdef`。条件必须是编译期常量。编译器只会编译条件为真的分支，另一个分支的代码甚至不需要是合法的（只要语法正确，类型检查会被跳过）。

```cpp
// 编译期条件
comptime if (sizeof(void^) == 8) {
    // 64位平台代码
    const int POINTER_SIZE = 8;
} else {
    // 32位平台代码
    const int POINTER_SIZE = 4;
}

// 泛型中的 comptime if
func foo<T>(T! value) {
    comptime if (sizeof(T) > 8) {
        // 仅当 T 大于 8 字节时编译此分支
        print("Large type");
        // 假设 large_process() 仅对大类型定义
        value.large_process(); 
    } else {
        // 否则编译此分支
        print("Small type");
    }
}
```

### 5.2 `comptime for` (Static For)

用于遍历编译期集合（如泛型参数包、反射字段列表、枚举成员）。编译器会将循环**展开 (Unroll)** 为一系列顺序语句。

```cpp
// 编译期循环
comptime for (var i = 0; i < 10; i++) {
    generateFunction<i>();
}

// 遍历类型
comptime for (var T in [int, float, double]) {
    implementTrait<T, Serializable>();
}

// 遍历类型的字段 (反射)
func print_fields<T>(T! obj) {
    // 遍历类型的字段 (反射)
    // 编译器会将此循环展开为多条 print 语句
    comptime for (var field : @T.fields) {
        print(field.name + ": " + obj.@[field]); 
    }
}
```

### 5.3 编译期 match

```cpp
// 编译期模式匹配
comptime var size = match (sizeof(void^)) {
    4 => "32-bit",
    8 => "64-bit",
    _ => "Unknown"
};
```

## 6. 编译期函数限制

### 6.1 允许的运算

编译期函数可以执行：
- 算术运算
- 逻辑运算
- 比较运算
- 位运算
- 控制流（if、for、while、match）
- 递归调用
- 内存分配（编译期堆）

### 6.2 禁止的运算

编译期函数不能执行：
- I/O 操作（文件、网络、控制台）
- 系统调用
- 运行时类型信息（除编译期反射）
- 多线程
- 动态链接

### 6.3 示例

```cpp
// 允许的编译期函数
comptime func fibonacci(int n) -> int {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 错误的编译期函数
comptime func readFile(string path) -> string {
    // 错误：不能在编译期读取文件
    return File.read(path);
}
```

## 7. 代码生成

### 7.1 生成函数

```cpp
// 生成序列化函数
comptime func generateSerializer<T>() {
    // 在编译期生成代码
    compile(R"
        func serialize(self) -> string {
            // 自动生成的序列化逻辑
        }
    ");
}

// 使用
class Point {
    float x, y;
    comptime generateSerializer<Point>();
}
```

### 7.2 生成类型

```cpp
// 生成变体类型
comptime func generateVariant<T...>() -> type {
    // 生成变体类型的代码
    return compile_type(R"
        class Variant {
            // 自动生成的变体实现
        }
    ");
}

// 使用
var MyVariant = generateVariant<int, float, string>;
```

## 8. 反射（预留）

### 8.1 类型反射

```cpp
// 获取类型信息
comptime var info = reflect<Point>;

// 遍历字段
comptime for (var field in info.fields) {
    print("Field: " + field.name + ", Type: " + field.type);
}

// 遍历方法
comptime for (var method in info.methods) {
    print("Method: " + method.name);
}
```

### 8.2 注解处理

```cpp
// 定义注解
annotation Serializable {
    string format = "json";
}

// 使用注解
@Serializable(format = "xml")
class Point {
    float x, y;
}

// 编译期处理注解
comptime {
    for (var cls in annotated_classes<Serializable>) {
        generateSerialization(cls);
    }
}
```

## 9. 编译期与运行时的交互

### 9.1 编译期计算，运行时存储

```cpp
// 编译期计算查找表
comptime var sin_table = generateSinTable<360>();

// 运行时快速查找
func fastSin(float angle) -> float {
    var index = int(angle) % 360;
    return sin_table[index];  // O(1) 查找
}
```

### 9.2 编译期配置

```cpp
// 编译期配置
comptime var config = parseConfigFile("config.json");

// 运行时行为由编译期配置决定
if (config.enable_logging) {
    log("Debug mode enabled");
}
```

## 10. 实现细节

### 10.1 编译期虚拟机

- C^ 编译器内置一个解释器/虚拟机
- 用于执行 `comptime` 代码
- 支持递归、循环、内存分配
- 有执行时间和内存限制

### 10.2 缓存机制

- 编译期计算结果可以缓存
- 避免重复计算相同的编译期表达式
- 加快增量编译速度

### 10.3 错误处理

```cpp
// 编译期错误
comptime func divide(int a, int b) -> int {
    if (b == 0) {
        compile_error("Division by zero in comptime");
    }
    return a / b;
}

// 编译期警告
comptime {
    if (sizeof(int) != 4) {
        compile_warning("Non-standard int size");
    }
}
```

## 11. 与 C++ 模板的比较

| 特性     | C++ 模板     | C^ comptime           |
| -------- | ------------ | --------------------- |
| 执行时机 | 编译期实例化 | 编译期执行            |
| 错误信息 | 复杂难懂     | 清晰明确              |
| 调试     | 困难         | 支持编译期调试        |
| 表达能力 | 图灵完备     | 图灵完备              |
| 语法     | 特殊语法     | 普通代码加 `comptime` |
| 类型操作 | 模板元编程   | 编译期反射            |

## 12. 最佳实践

### 12.1 使用编译期计算优化

```cpp
// 推荐：编译期计算查找表
comptime var lookup_table = generateTable<256>();

// 不推荐：运行时计算
func calculate(int x) -> int {
    return expensiveCalculation(x);  // 每次调用都计算
}
```

### 12.2 避免过度使用

```cpp
// 不推荐：简单的常量不需要 comptime
comptime var one = 1;  // 过度

// 推荐：直接使用 const
const int ONE = 1;
```

### 12.3 编译期验证

```cpp
// 编译期验证配置
comptime {
    if (MAX_SIZE <= 0) {
        compile_error("MAX_SIZE must be positive");
    }
    
    if (VERSION_MAJOR < 1) {
        compile_warning("Pre-release version");
    }
}
```

## 13. 版本历史

| 版本 | 日期    | 变更     |
| ---- | ------- | -------- |
| 1.0  | 2025-02 | 初始版本 |
