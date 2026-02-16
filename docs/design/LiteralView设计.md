# LiteralView 设计文档

## 概述

`LiteralView` 是 C^ 语言的**内置原生类型 (Primitive Type)**，用于承载字符串字面量等静态只读数据。它是编译器与标准库之间的桥梁，实现了零拷贝构造与类型安全。

---

## 1. 核心定义

### 1.1 类型声明

`LiteralView` 是编译器内置原生类型，类似于 `int`，不需要定义 struct：

```cpp
// 编译器内置原生类型（不需要定义 struct，它就像 int 一样存在）
// 伪代码表示其内部结构：
struct LiteralView {
    private byte!^  ptr_;  // 指向 .rodata 中的字符串 默认带\0 终止符
    private long  len_;    // 字符串长度（不含 null 终止符）
    
    // ptr 和 len 是只读的 get 属性，不能放在赋值左边
    public get ptr -> byte!^=> ptr_;  // 只读属性：不能写 s.ptr = xxx
    public get len -> long=> len_;     // 只读属性：不能写 s.len = xxx
}
```

### 1.2 为什么设计为内置类型？

1.  **零依赖**：用户在编写自定义标准库（如嵌入式环境）时，可以直接使用 `LiteralView` 来处理字面量，而不需要定义一个特殊的结构体
2.  **统一性**：它作为连接"编译器物理世界"（地址+长度）与"用户类型系统"（String/Slice）的唯一桥梁
3.  **去魔法化**：编译器不需要去查找名为 `std.LiteralView` 的特定结构体

### 1.3 物理布局

`LiteralView` 的内存布局是固定的：
```
+------------------+------------------+
| ptr (8 bytes)    | len (8 bytes)    |
+------------------+------------------+
```
- 总大小：16 字节（64 位平台）
- 栈上存储，不占用堆内存

### 1.4 语义特点

1.  **只读属性（Get Only）**：
    - `ptr` 和 `len` 是只读的 get 属性
    - **不能**放在赋值左边：`s.ptr = xxx` → 编译错误
    - **可以**读取：`let p = s.ptr` → 正确
2.  **数据只读**：`ptr` 指向的是 `.rodata` 段的只读数据
3.  **零拷贝**：所有视图转换都是零拷贝的
4.  **静态生命周期**：数据来自 `.rodata`，生命周期是静态的
5.  **不可修改**：不能通过 `LiteralView` 修改数据

---

## 2. 类型推导

编译器遇到字符串字面量 `"..."` 时，无条件将其类型推导为 `LiteralView`：

```cpp
var s1 = "Hello";          // s1: LiteralView
let s2 = "World, C^!";     // s2: LiteralView
```

---

## 3. 隐式转换

### 3.1 转换链

`LiteralView` 支持以下安全的隐式转换（由标准库通过 `implicit operator` 提供）：

```
LiteralView
    |
    +-- 隐式转换 --> StringView (byte![])
    |                      |
    |                      +-- 隐式转换 --> byte!^ (C 风格指针)
    |
    +-- 隐式转换 --> byte!^ (直接转换，方便 C 互操作)
    |
    +-- 构造 --> String (所有者)
```

### 3.2 标准库扩展实现

```cpp
// std/core/StringView.ch (标准库)

extension LiteralView {
    // 隐式转换为 StringView (byte![])
    public implicit operator byte![]() {
        return { ptr = self.ptr, len = self.len };
    }
    
    // 隐式转换为 byte!^ (C 风格指针)
    public implicit operator byte!^() {
        return self.ptr;
    }
    
    // 便捷方法
    public func isEmpty() -> bool {
        return self.len == 0;
    }
    
    public func toDebugString() -> string {
        return "LiteralView(ptr=" + self.ptr + ", len=" + self.len + ")";
    }
}
```

### 3.3 示例

#### 场景 1：引入标准库（推荐）

```cpp
import std;

extern "C" {
    func printf(byte^ format, ...) -> int;
    func puts(byte^ s) -> int;
}

func main() -> int {
    // 1. 直接传递给 C 函数（隐式转换为 byte^）
    puts("Hello, World!");
    printf("The answer is %d\n", 42);
    
    // 2. 赋值给 StringView（隐式转换为 byte![]）
    byte![] s = "Hello, C^!";
    puts(s);  // 再次隐式转换为 byte^
    
    // 3. 显式使用属性
    var msg = "Hello";
    println("Length: " + msg.len);
    println("Ptr: " + msg.ptr);
    
    return 0;
}
```

#### 场景 2：不引入标准库（裸机/嵌入式）

```cpp
// 不引入标准库

extern "C" {
    func puts(byte^ s) -> int;
}

func main() -> int {
    // 必须显式调用 .ptr
    puts("Hello, World!".ptr);
    
    // 也可以访问其他属性
    var len = "Hello".len;
    
    return 0;
}
```

---

## 4. 与其他类型的对比

| 类型          | 所有者 | 可修改 | 内存位置  | 用途             |
| ------------- | ------ | ------ | --------- | ---------------- |
| `LiteralView` | 编译器 | ❌ 否   | `.rodata` | 承载字符串字面量 |
| `byte![]`     | 用户   | ❌ 否   | 任意      | 只读字节视图     |
| `byte[]`      | 用户   | ✅ 是   | 栈/堆     | 可写字节视图     |
| `String`      | 用户   | ✅ 是   | 堆        | 拥有内存的字符串 |

---

## 5. 与 String 的集成

### 5.1 零拷贝构造

`String` 类通过重载构造函数来区分字面量和其他字符串源：

```cpp
class String {
    // ...
    
    // 针对字面量的优化构造：零拷贝 (Static Mode)
    public String(LiteralView lv) {
        self.data = lv.ptr;
        self.len = lv.len;
        self.capacity = FLAG_STATIC; // 标记为静态模式，析构时不释放
    }
    
    // 针对普通切片的构造：深拷贝
    public String(StringView sv) {
        allocate_and_copy(sv.data, sv.len);
    }
}

// 用法
String s1 = "hello"; // 匹配 String(LiteralView)，零拷贝
String s2 = get_slice(); // 匹配 String(StringView)，深拷贝
```

### 5.2 数组初始化

```cpp
// 栈数组：拷贝数据
byte[$] buffer = "hello";  // 在栈上分配 6 字节并拷贝数据

// 只读切片：零拷贝
byte![] view = "hello";    // 零拷贝，指向 .rodata
```

---

## 6. 操作符重载与字符串拼接

### 6.1 拼接支持

为了支持直观的字符串拼接语法（如 `"hello" + 123`），`LiteralView` 必须支持 `+` 操作符。由于 `LiteralView` 是不可变的静态视图，任何拼接操作都将产生一个新的 `String` 对象（分配堆内存）。

```cpp
// 1. 字面量 + 字面量 -> String
public static func operator+(LiteralView lhs, LiteralView rhs) -> String {
    String result = String.allocate(lhs.len + rhs.len);
    result.append(lhs);
    result.append(rhs);
    return result;
}

// 2. 针对基本类型的零开销重载 (由标准库直接提供)
public static func operator+(LiteralView lhs, int rhs) -> String { 
    return String(lhs) + std.to_string(rhs); 
}
public static func operator+(LiteralView lhs, float rhs) -> String { 
    return String(lhs) + std.to_string(rhs); 
}
public static func operator+(LiteralView lhs, bool rhs) -> String { 
    return String(lhs) + (rhs ? "true" : "false"); 
}

// 示例
var s1 = "hello" + 10;      // "hello10"
var s2 = "Value: " + 3.14;  // "Value: 3.14"
```

### 6.2 运算符优先级规则

当编译器遇到 `"..." + x` 时，会按照以下顺序查找 `operator+` 的定义：

1.  **Rank 1 (最高)**: 显式导入/当前作用域
2.  **Rank 2**: 目标类型推导 (Target Typing)
3.  **Rank 3 (最低)**: 标准库默认

---

## 7. 错误提示

当没有引入标准库时，编译器给出清晰的错误提示：

```
error: cannot convert 'LiteralView' to 'byte^' implicitly
  --> hello.ch:6:10
   |
 6 |     puts("Hello, World!");
   |          ^^^^^^^^^^^^^^^^
   |
   = note: 'LiteralView' cannot be implicitly converted to 'byte^'
   = help: consider importing the standard library with 'import std;'
   = help: or explicitly use '.ptr': 'puts("Hello, World!".ptr)'
```

---

## 8. 设计优势

1.  **零开销**：`LiteralView` 本身只有 16 字节，不占用额外内存
2.  **类型安全**：通过类型系统防止修改只读数据
3.  **灵活转换**：可以零拷贝地转换为多种视图类型
4.  **清晰语义**：明确区分"字面量"和"普通数据"
5.  **标准库友好**：编译器只负责最基础的功能，高级功能由标准库实现
6.  **用户选择自由**：可选择是否引入标准库

---

## 9. 编译器实现要点

### 9.1 字面量类型推导

```cpp
// SemanticAnalyzer.cpp
std::shared_ptr<types::Type>
SemanticAnalyzer::analyzeLiteralExpr(std::unique_ptr<ast::Literal> literal) {
    switch (literal->type) {
        case ast::Literal::Type::String:
            // 返回内置的 LiteralView 类型
            return types::TypeFactory::getLiteralViewType();
            
        // ... 其他字面量类型
    }
}
```

### 9.2 类型兼容性检查

```cpp
// 类型兼容性检查伪代码
bool isCompatibleWith(const Type& other) const {
    if (this->isLiteralView()) {
        // 情况 1：对方是 byte!^
        if (other.isPointer() && other.getPointeeType()->isByte()) {
            if (hasExtensionImplicitOperator("LiteralView", "byte!^")) {
                return true;
            }
            return false;
        }
        
        // 情况 2：对方是 byte![]
        if (other.isSlice() && other.getElementType()->isByte()) {
            if (hasExtensionImplicitOperator("LiteralView", "byte![]")) {
                return true;
            }
            return false;
        }
    }
    // ...
}
```

---

## 10. 终极统一：字符串与字节切片的生态闭环

### 10.1 物理真理：一切皆字节

在 C^ 中，字符串字面量 `"hello"` 的物理本质是：
1.  **存储**：UTF-8 编码的字节序列。
2.  **位置**：静态只读数据段 (`.rodata`)。
3.  **原生类型**：`LiteralView` (纯粹的地址+长度，无语义)。

### 10.2 语义分叉：数据 vs 文本

`LiteralView` 处于"叠加态"，它可以无损地坍缩为两种视图：

1.  **数据视图 (`byte![]`)**：
    *   **语义**：我看待它只是一堆二进制数据。
    *   **操作**：索引、切片、内存拷贝、CRC校验。
    *   **转换**：`LiteralView` -> `byte![]` (隐式，零开销)。

2.  **文本视图 (`StringView`)**：
    *   **语义**：我看待它是有意义的文本 (UTF-8 Text)。
    *   **操作**：正则匹配、子串查找、大小写转换。
    *   **转换**：`LiteralView` -> `StringView` (隐式，零开销)。

### 10.3 完整生态图谱

| 场景                | 代码                    | 发生了什么？                             | 内存位置   | 读写权限   |
| :------------------ | :---------------------- | :--------------------------------------- | :--------- | :--------- |
| **只读视图 (数据)** | `byte![] b = "abc";`    | `LiteralView` 隐式转为 `Slice<byte!>`    | .rodata    | **只读**   |
| **只读视图 (文本)** | `StringView s = "abc";` | `LiteralView` 隐式转为 `StringView`      | .rodata    | **只读**   |
| **可变副本 (栈)**   | `byte[$] a = "abc";`    | 从 .rodata **拷贝**到栈数组              | Stack      | **可读写** |
| **可变副本 (堆)**   | `String s = "abc";`     | 从 .rodata **拷贝**到堆 (或 SSO)         | Heap/Stack | **可读写** |
| **非法操作**        | `byte[] x = "abc";`     | **编译错误**！试图用可变切片指向只读内存 | N/A        | N/A        |

---

## 11. 总结

| 维度                 | 评价                                |
| :------------------- | :---------------------------------- |
| **核心不依赖标准库** | ✅ LiteralView 是内置类型            |
| **用户体验**         | ✅ 引入标准库后自动隐式转换          |
| **可见性**           | ✅ 用户能清楚看到 LiteralView 的成员 |
| **灵活性**           | ✅ 可选择是否使用标准库              |
| **学习曲线**         | ✅ 概念清晰，分层明确                |
| **错误提示友好**     | ✅ 可提供明确的帮助信息              |

这个方案既保证了语言核心的独立性，又提供了优秀的用户体验，是最佳平衡！
