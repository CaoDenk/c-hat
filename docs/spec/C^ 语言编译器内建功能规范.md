# C^ 语言编译器内建功能规范

本文档详细定义 C^ 编译器提供的内建功能，包括内置属性（Attributes）和编译器特殊类型（Compiler Intrinsic Types）。这些功能通常用于向编译器提供元数据、优化提示或实现语言的核心底层机制。

## 1. 内置属性 (Attributes)

C^ 使用 `[attribute]` 语法来标记声明，以提供额外的编译器指令。

### 1.1 `deprecated`

用于标记已弃用的类型、函数、变量或成员。

*   **语法**：`[deprecated]` 或 `[deprecated("reason")]`
*   **行为**：
    *   当代码引用被标记的实体时，编译器会发出警告。
    *   可选的字符串参数用于说明弃用原因或替代方案。

```cpp
[deprecated("Use 'newFunction' instead")]
func oldFunction() { ... }

func main() {
    oldFunction(); // 编译器警告: 'oldFunction' is deprecated: Use 'newFunction' instead
}
```

### 1.2 `inline` / `noinline`

用于控制函数的内联行为。

*   **`[inline]`**：建议编译器将函数内联展开。这只是一个提示，编译器可能会忽略（例如对于递归函数）。
*   **`[noinline]`**：强制编译器**不**内联该函数，即使它很短。常用于调试或冷代码路径（如异常抛出辅助函数）。

```cpp
[inline]
func add(int a, int b) -> int {
    return a + b;
}

[noinline]
func throwError() {
    throw Error("...");
}
```

### 1.3 `noreturn`

标记函数不会返回控制权到调用者（例如无限循环、进程终止或抛出异常）。

*   **语法**：`[noreturn]`
*   **用途**：帮助编译器进行控制流分析（消除死代码警告）和优化。

```cpp
[noreturn]
func panic(string msg) {
    print(msg);
    exit(1);
}
```

### 1.4 `likely` / `unlikely`

用于分支预测优化，标记某个分支执行的概率。

*   **语法**：通常用于 `if` 或 `match` 的分支上（注：具体放置位置取决于语法支持，通常置于语句块前或 case 标签上）。

```cpp
if (ptr == null) [unlikely] {
    // 错误处理路径，编译器会将其放置在冷代码区
    handleError();
}
```

### 1.5 `align`

指定变量或类型的内存对齐方式。

*   **语法**：`[align(N)]`，其中 N 必须是 2 的幂。

```cpp
[align(16)]
struct Vector4 {
    float x, y, z, w;
}
```

## 2. 编译器内置类型 (Compiler Intrinsic Types)

这些类型由编译器特殊识别，并在语言语义中扮演关键角色。它们通常在 `std.core` 中定义，但其行为由编译器直接支持。

### 2.1 `literal_view`

`literal_view` 是 C^ 实现高效字符串处理的核心机制。它专门用于表示**编译期字符串字面量**。

#### 2.1.1 定义

在标准库中，它的定义类似于：

```cpp
namespace std {
    // 编译器保证：ptr 指向静态只读数据区 (.rodata)，生命周期为整个程序运行期
    public struct literal_view {
        byte!^ ptr;
        usize length;
    }
}
```

#### 2.1.2 编译器行为

1.  **字面量类型推导**：
    *   当编译器遇到字符串字面量 `"hello"` 时，它不仅仅是一个 `byte` 数组或指针。
    *   在函数重载决议中，字符串字面量优先匹配 `literal_view` 类型参数。
    *   如果上下文需要，它也可以隐式转换为 `byte![]` (slice) 或 `cstring`。

2.  **构造函数重载与零拷贝**：
    *   `std::string` 利用这一特性实现了**零拷贝构造**。
    *   普通字符串构造 `string(byte![])` 通常需要深拷贝（因为切片的生命周期未知）。
    *   `string(literal_view)` 构造函数知道数据源位于 `.rodata`（静态区），因此可以跳过分配和拷贝，直接存储指针并标记为 "Static Mode"。

#### 2.1.3 示例

```cpp
class string {
    // ... 内部状态 ...
    
    // 1. 普通切片构造：必须拷贝，因为 sv 可能指向栈变量
    public string(string_view sv) {
        allocate_and_copy(sv.data, sv.length);
    }

    // 2. 字面量构造：零拷贝！
    public string(literal_view lv) : data(lv.ptr), len(lv.length), cap(FLAG_STATIC) {
        // Static Mode: cap 包含 FLAG_STATIC，析构函数检查此标志跳过释放
    }
}

func main() {
    // 编译器选择 string(literal_view) 重载
    // 结果：s1 内部直接指向 .rodata 中的 "hello"，无堆分配
    string s1 = "hello"; 
    
    var local_arr = "world".to_bytes();
    // 编译器选择 string(string_view) 重载
    // 结果：s2 必须在堆上分配并拷贝 "world"
    string s2 = local_arr; 
}
```

### 2.2 `TypeInfo`

用于支持运行时类型信息和反射。

*   **获取方式**：通过 `@` 操作符（如 `@int`, `@myVar`）。
*   **用途**：提供类型名称、大小、对齐、成员列表等元数据（编译期常量）。

### 2.3 `InitializerList<T>`

用于支持花括号初始化列表语法 `{ ... }`。

*   **语法**：`var x = {1, 2, 3};`
*   **行为**：编译器创建一个临时的数组，并将其封装为 `InitializerList<T>` 传递给构造函数。

## 3. 编译器内置函数 (Intrinsics)

编译器提供了一些特殊的函数，直接映射到机器指令或编译器内部逻辑。

*   `sizeof(T)`: 返回类型大小（编译期常量）。
*   `typeof(expr)`: 返回表达式类型（编译期）。
*   `__debug_break()`: 触发调试器断点。
*   `__unreachable()`: 提示编译器此代码路径不可达（用于优化）。
