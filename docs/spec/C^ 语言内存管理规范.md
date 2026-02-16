# C^ 语言内存管理规范

本文档详细定义 C^ 语言的内存管理机制，包括堆内存分配、RAII、所有权系统和生命周期管理。

## 1. 内存模型概述

C^ 语言采用**手动内存管理** + **RAII** + **所有权系统**的组合：

- **栈内存**：自动分配和释放，用于局部变量
- **堆内存**：手动分配（`new`）和释放（`delete`）
- **静态内存**：程序启动时分配，程序结束时释放
- **RAII**：资源获取即初始化，自动管理资源生命周期

## 2. 栈内存管理

### 2.1 分配规则

- 局部变量在声明时分配栈内存
- 变量在作用域结束时自动释放
- 编译期确定栈帧大小

### 2.2 延迟初始化 (late)

`late` 是 C^ 的核心优化特性，用于在不确定是否初始化的情况下预留栈空间，解决异常安全和条件初始化难题。

*   **栈上预留**：`late T var;` 在栈帧上分配 `sizeof(T)` 空间，但不调用构造函数。
*   **RVO 配合**：在 `try-catch` 或条件分支中赋值时，触发 RVO（返回值优化），直接在预留空间构造，避免临时对象拷贝。
*   **状态追踪**：编译器会追踪 `late` 变量的初始化状态。

#### 语法

```
late_declaration ::= 'late' type identifier ';'
late_initialization ::= identifier '=' expression ';'
```

#### 示例

```cpp
// 基本用法
func process_data() {
    late DatabaseConnection db; // 仅分配内存，不连接
    
    try {
        // 在 try 块内初始化
        db = connect_to_db("localhost")~; 
        // 此时 db 已构造，可以直接使用
        db.query("SELECT * FROM users");
    } catch (NetworkError! e) {
        // 若初始化失败，db 仍处于未构造状态，析构函数不会被调用
        print("Connection failed");
        return;
    }
    
    // 出了 try 块，db 依然有效（如果在 try 中成功初始化）
    // 编译器确保此时 db 是已初始化的，或者是不可达的
}

// 条件初始化
func conditional_init(bool use_fast_path) {
    late Result result;
    
    if (use_fast_path) {
        result = fast_path()~; // 直接在预留空间构造
    } else {
        result = slow_path()~; // 直接在预留空间构造
    }
    
    // result 在这里一定已初始化
    return result;
}
```

#### 语义规则

1. `late` 声明的变量在栈上预留空间，但不调用构造函数
2. 首次赋值时，编译器确保在预留空间上构造（RVO）
3. 编译器追踪所有代码路径，确保在使用前已初始化
4. 如果初始化失败（如抛出异常），变量保持未构造状态，不会调用析构函数
5. 如果成功初始化，作用域结束时自动调用析构函数

### 2.3 示例

```cpp
func example() {
    var x = 5;        // 栈上分配 int
    var arr = [1, 2, 3];  // 栈上分配数组
    
    {
        var y = 10;   // 内层作用域
    }  // y 在这里释放
    
}  // x 和 arr 在这里释放
```

### 2.3 栈溢出保护

- 编译器可以插入栈溢出检查
- 大数组应该分配在堆上
- 递归深度需要控制

## 3. 堆内存管理

### 3.1 new 表达式

#### 语法

```
new_expression ::= 'new' type ('(' expression ')')?
                 | 'new' type '[' expression ']'
```

#### 语义

- 在堆上分配指定类型的内存
- 返回指向分配内存的指针
- 单对象形式返回 `Type^`
- 数组形式返回 `Type[]`（切片）

#### 示例

```cpp
// 分配单个对象
int^ p1 = new int(42);        // 初始化为 42
int^ p2 = new int;            // 默认初始化（0）

// 分配数组
int[] arr1 = new int[10];     // 10个int，默认初始化
int[] arr2 = new int[5] {1, 2, 3, 4, 5};  // 初始化列表

// 分配类对象
Point^ pt = new Point(1.0, 2.0);
```

### 3.2 delete 表达式

#### 语法

```
delete_expression ::= 'delete' expression
```

#### 语义

- 释放 `new` 分配的内存
- 调用对象的析构函数（如果有）
- 对于数组，释放所有元素
- 删除后指针/切片变为无效

#### 示例

```cpp
// 删除单个对象
int^ p = new int(42);
delete p;
// p 现在悬空，不应再使用

// 删除数组
int[] arr = new int[10];
delete arr;

// 删除类对象
Point^ pt = new Point(1.0, 2.0);
delete pt;  // 调用 ~Point()
```

### 3.3 内存分配失败

- 内存不足时 `new` 抛出 `OutOfMemoryError`
- 不支持 `nothrow new`（使用异常处理）

```cpp
try {
    var huge = new byte[1024 * 1024 * 1024 * 1024];  // 1TB
} catch (OutOfMemoryError e) {
    print("Not enough memory");
}
```

## 4. 指针类型

### 4.1 非空指针

```cpp
int^ p;  // 非空指针，必须初始化
int^ q = null;  // 错误：非空指针不能为 null

// 正确用法
int x = 5;
int^ p = ^x;  // 指向栈变量
int^ q = new int(5);  // 指向堆变量
```

### 4.2 可空指针

```cpp
Optional<int^> p;  // 可空指针，可以为 null
Optional<int^> q = null;  // OK

// 安全解引用
if (p.has_value()) {
    var value = p.value()^;  // 安全
}

// 使用 match
match (p) {
    None => print("null"),
    Some(val) => print(val^)
}
```

### 4.3 指针运算

C^ 不支持指针算术（除了数组索引）：

```cpp
int[] arr = new int[10];
var first = arr[0];   // OK
var second = arr[1];  // OK

// int^ p = arr;
// p++;  // 错误：不支持指针算术
```

## 5. RAII 机制

### 5.1 构造函数

- 对象创建时自动调用
- 用于初始化资源

```cpp
class File {
    int fd;
    
    public File(string path) {
        fd = open(path);
        if (fd < 0) {
            throw FileOpenError(path);
        }
    }
}
```

### 5.2 析构函数

- 对象销毁时自动调用
- 用于释放资源
- 使用 `~` 前缀

```cpp
class File {
    int fd;
    
    public ~ {
        if (fd >= 0) {
            close(fd);
        }
    }
}
```

### 5.3 使用示例

```cpp
func readFile(string path) -> string {
    var file = File(path);  // 调用 init
    defer file.~();         // 确保析构（可选，RAII自动处理）
    
    return file.readAll();
}  // file 在这里自动销毁，调用 ~File()
```

## 6. 所有权系统

### 6.1 所有权规则

1. 每个值有且只有一个所有者
2. 当所有者离开作用域，值被销毁
3. 所有权可以转移（move）

### 6.2 所有权转移

```cpp
class UniquePtr<T> {
    T^ ptr;
    
    public func init(T^ p) {
        ptr = p;
    }
    
    public ~ {
        delete ptr;
    }
    
    // 禁止拷贝
    delete func UniquePtr(UniquePtr& other);
    delete func operator=(UniquePtr& other);
    
    // 允许移动
    public  UniquePtr(UniquePtr~ other) {
        ptr = other.ptr;
        other.ptr = null;
    }
}

// 使用
func example() {
    var p1 = UniquePtr<int>(new int(42));
    var p2 = p1~;  // 移动所有权到 p2
    // p1 现在无效
}
```

### 6.3 借用（引用）

```cpp
func printValue(int& value) {  // 借用
    print(value);
}  // 不获取所有权

func example() {
    var x = 5;
    printValue(x);  // 借用 x
    print(x);       // x 仍然有效
}
```

## 7. 智能指针（标准库）

### 7.1 unique_ptr

独占所有权，不可拷贝，可移动。

```cpp
import std.memory;

func example() {
    var p = unique_ptr<int>(new int(42));
    print(p^);  // 解引用
    
    var q = p~;  // 移动所有权
    // p 现在为空
    print(q^);   // OK
}
```

### 7.2 shared_ptr

共享所有权，引用计数。

```cpp
import std.memory;

func example() {
    var p1 = shared_ptr<int>(new int(42));
    {
        var p2 = p1;  // 引用计数 +1
        print(p2^);   // OK
    }  // 引用计数 -1
    
    print(p1^);  // OK，仍然有效
}
```

### 7.3 weak_ptr

弱引用，不增加引用计数。

```cpp
import std.memory;

func example() {
    var shared = shared_ptr<int>(new int(42));
    var weak = weak_ptr<int>(shared);
    
    if (var locked = weak.lock()) {
        print(locked^);  // 安全使用
    }
}
```

## 8. 内存安全规则

### 8.1 禁止的操作

```cpp
// 1. 使用悬空指针
int^ dangling() {
    var x = 5;
    return ^x;  // 错误：返回栈变量的地址
}

// 2. 重复释放
int^ p = new int(5);
delete p;
delete p;  // 错误：重复释放

// 3. 内存泄漏
func leak() {
    var p = new int(5);
    // 没有 delete，内存泄漏
}

// 4. 越界访问
int[] arr = new int[10];
var x = arr[10];  // 错误：越界
```

### 8.2 安全检查（编译期）

```cpp
// 编译器可以检测的问题：

// 1. 未初始化变量
int^ x;
print(x^);  // 错误：x 未初始化

// 2. 可能的空指针解引用
Optional<int^> p = getPointer();
if (p) {
    print(p.value()^);
} else {
    // 处理空指针
}

// 3. 所有权错误
var p1 = unique_ptr<int>(new int(5));
var p2 = p1;  // 错误：unique_ptr 不可拷贝
```

## 9. defer 与资源管理

### 9.1 基本用法

```cpp
func processFile(string path) {
    var file = File.open(path);
    defer file.close();  // 确保关闭
    
    // 处理文件...
    if (error) {
        return;  // file.close() 自动调用
    }
    
}  // file.close() 自动调用
```

### 9.2 多个 defer

```cpp
func example() {
    var resource1 = acquire1();
    defer release1(resource1);
    
    var resource2 = acquire2();
    defer release2(resource2);
    
    var resource3 = acquire3();
    defer release3(resource3);
    
    // 执行顺序：release3, release2, release1
}
```

### 9.3 defer 与异常

```cpp
func riskyOperation() {
    var resource = acquire();
    defer release(resource);  // 即使抛出异常也会执行
    
    mightThrow();  // 可能抛出异常
}
```

## 10. 内存布局

### 10.1 对象布局

```cpp
class Example {
    int a;      // 4字节
    double b;   // 8字节
    bool c;     // 1字节
    // 可能有填充
}

// 内存布局（假设64位）：
// [a: 4字节][填充: 4字节][b: 8字节][c: 1字节][填充: 7字节]
// 总计：24字节
```

### 10.2 对齐要求

```cpp
// 可以使用 alignas 指定对齐
alignas(16) class Vector4 {
    float x, y, z, w;
}

// 检查对齐
static_assert(alignof(Vector4) == 16);
```

## 11. 与 C/C++ 的区别

| 特性     | C/C++                           | C^                     |
| -------- | ------------------------------- | ---------------------- |
| 内存分配 | `malloc`/`free`, `new`/`delete` | `new`/`delete`         |
| 指针算术 | 支持                            | 不支持（数组索引除外） |
| 智能指针 | 标准库                          | 标准库                 |
| RAII     | 支持                            | 支持（更严格）         |
| 垃圾回收 | 无                              | 无                     |
| 空指针   | `nullptr`                       | `null`                 |
| 可空类型 | 无                              | `Optional<T^>`         |
| defer    | 无                              | 支持                   |

## 12. 最佳实践

### 12.1 使用 RAII

```cpp
// 推荐：使用 RAII
func processFile(string path) {
    var file = File(path);  // RAII
    // 自动管理资源
}

// 不推荐：手动管理
func processFile(string path) {
    var fd = open(path);
    // 容易忘记关闭
    close(fd);
}
```

### 12.2 使用智能指针

```cpp
// 推荐：使用智能指针
func example() {
    var p = unique_ptr<int>(new int(42));
    // 自动释放
}

// 不推荐：原始指针
func example() {
    var p = new int(42);
    // 容易忘记 delete
}
```

### 12.3 避免内存泄漏

```cpp
// 使用 defer 确保释放
func example() {
    var p = new int(42);
    defer delete p;
    
    // 即使提前返回或抛出异常，p 也会被释放
}
```

## 13. 版本历史

| 版本 | 日期    | 变更     |
| ---- | ------- | -------- |
| 1.0  | 2025-02 | 初始版本 |
