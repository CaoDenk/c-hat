# C^ 语言标准库规范

本文档详细定义 C^ 语言标准库的接口、行为和实现要求。

## 1. 概述

C^ 标准库分为以下模块：

- **core**：核心类型和函数
- **io**：输入输出
- **memory**：内存管理
- **string**：字符串处理
- **container**：容器（数组、列表、映射等）
- **algorithm**：算法
- **math**：数学函数
- **time**：时间和日期
- **thread**：多线程（预留）
- **net**：网络（预留）

## 2. 设计哲学

C^ 标准库遵循以下核心设计原则：

1. **零成本抽象**：所有抽象在零开销的前提下提供便利性
2. **显式分配**：明确区分栈分配、堆分配和视图
3. **类型安全**：通过类型系统防止常见错误
4. **UTF-8 优先**：字符串默认使用 UTF-8 编码
5. **机制在语言，策略在库**：语言提供核心机制，标准库提供具体实现

## 3. 核心模块（std.core）

### 3.1 基本类型别名

C^ 使用 `using` 关键字创建类型别名。标准库规范中使用 `using` 定义所有基本类型的别名，`type` 关键字作为兼容性语法糖存在，新代码应优先使用 `using`。

```cpp
module std.core;

// 整数别名
using int8 = sbyte;
using int16 = short;
using int32 = int;
using int64 = long;

using uint8 = byte;
using uint16 = ushort;
using uint32 = uint;
using uint64 = ulong;

// 浮点别名
using f32 = float;
using f64 = double;
using f16 = fp16;
using bf16 = bf16;

// 字符别名
using char8 = byte;    // UTF-8 代码单元
using char16 = ushort; // UTF-16 代码单元
using char32 = char;   // UTF-32 码点（等同于 char）

// 大小类型
using size = ulong;    // 无符号大小（等同于 usize）
using ssize = long;    // 有符号大小（等同于 isize）
using ptrdiff = long;  // 指针差值

// C 互操作类型
using cstring = byte^; // 指向以 null 结尾的字节序列
```

### 3.2 字符串视图类型

```cpp
// string_view 是 byte![] 的类型别名
// 它是一个非拥有的 UTF-8 字符串切片
public using string_view = byte![];

// literal_view 是字符串字面量的原生类型
// 它指向 .rodata 中的静态常量
public struct literal_view {
    byte!^ ptr;
    usize length;
}
```

### 3.3 工具函数

```cpp
// 类型转换
func to<T>(value) -> T;
func as<T>(value) -> T;
func cast<T>(value) -> T;

// 最小/最大值
func min<T>(T a, T b) -> T where T : Comparable;
func max<T>(T a, T b) -> T where T : Comparable;
func clamp<T>(T value, T min, T max) -> T where T : Comparable;

// 交换
func swap<T>(T& a, T& b);

// 移动
func move<T>(T& value) -> T~;

// 完美转发（预留）
func forward<T>(T& value) -> T&;
func forward<T>(T~ value) -> T~;
```

### 3.4 错误处理

```cpp
// 基础错误类型
public class Error {
    public string message;
    
    public Error(string msg) {
        message = msg;
    }
    
    public func what() -> string {
        return message;
    }
}

// 常见错误类型
public class OutOfRangeError : Error {
    public OutOfRangeError(string msg) : base(msg) {}
}

public class InvalidArgumentError : Error {
    public InvalidArgumentError(string msg) : base(msg) {}
}

public class NullPointerError : Error {
    public NullPointerError() : base("Null pointer dereference") {}
}

public class OutOfMemoryError : Error {
    public OutOfMemoryError() : base("Out of memory") {}
}

// Result 类型用于错误传播
public class Result<T, E = Error> {
    private bool is_ok;
    private union {
        T ok_value;
        E err_value;
    }
    
    public Result(T value) {
        is_ok = true;
        ok_value = value;
    }
    
    public Result(E error) {
        is_ok = false;
        err_value = error;
    }
    
    public func is_ok(self) -> bool {
        return is_ok;
    }
    
    public func is_err(self) -> bool {
        return !is_ok;
    }
    
    public func unwrap(self) -> T {
        if (is_ok) return ok_value;
        panic(err_value.what());
    }
    
    public func unwrap_or(self, T default_value) -> T {
        if (is_ok) return ok_value;
        return default_value;
    }
    
    public func ok(self) -> T? {
        if (is_ok) return ok_value;
        return null;
    }
    
    public func err(self) -> E? {
        if (!is_ok) return err_value;
        return null;
    }
}
```

## 4. 内存模块（std.memory）

### 4.1 智能指针

```cpp
module std.memory;

// unique_ptr - 独占所有权
public class unique_ptr<T> {
    private T^ ptr;
    
    public unique_ptr();
    public unique_ptr(T^ p);
    public unique_ptr(unique_ptr~ other);
    public ~unique_ptr();
    
    // 禁止拷贝
    delete unique_ptr(unique_ptr& other);
    delete func operator=(unique_ptr& other);
    
    // 操作
    public func operator^(self) -> T&;
    public func get(self) -> T^;
    public func release(self) -> T^;
    public func reset(self, T^ p = null);
    public func swap(self, unique_ptr& other);
    
    // 检查
    public func operator bool(self) -> bool;
}

// shared_ptr - 共享所有权
public class shared_ptr<T> {
    private T^ ptr;
    private ControlBlock^ control;
    
    public shared_ptr();
    public shared_ptr(T^ p);
    public shared_ptr(shared_ptr& other);
    public shared_ptr(shared_ptr~ other);
    public ~shared_ptr();
    
    public func operator=(self, shared_ptr& other) -> shared_ptr&;
    
    public func operator^(self) -> T&;
    public func get(self) -> T^;
    public func use_count(self) -> long;
    public func reset(self, T^ p = null);
    public func swap(self, shared_ptr& other);
    
    public func operator bool(self) -> bool;
}

// weak_ptr - 弱引用
public class weak_ptr<T> {
    private T^ ptr;
    private ControlBlock^ control;
    
    public weak_ptr();
    public weak_ptr(shared_ptr& other);
    public weak_ptr(weak_ptr& other);
    public ~weak_ptr();
    
    public func operator=(self, weak_ptr& other) -> weak_ptr&;
    
    public func lock(self) -> shared_ptr<T>;
    public func expired(self) -> bool;
    public func use_count(self) -> long;
    public func reset(self);
    public func swap(self, weak_ptr& other);
}

// make 函数
public func make_unique<T>(Args... args) -> unique_ptr<T>;
public func make_shared<T>(Args... args) -> shared_ptr<T>;
```

### 4.2 分配器接口

```cpp
// 分配器概念
public concept Allocator {
    requires func allocate(size n) -> byte^;
    requires func deallocate(byte^ p, size n);
    requires func max_size() -> size;
}

// 默认分配器
public class default_allocator {
    public func allocate(size n) -> byte^ {
        return new byte[n];
    }
    
    public func deallocate(byte^ p, size n) {
        delete[] p;
    }
    
    public func max_size() -> size {
        return size.max;
    }
}
```

## 5. 字符串模块（std.string）

### 5.1 设计说明

C^ 的字符串系统基于以下核心设计：

1. **`string` 是标准库类型**：不是语言内置的"魔法"类型
2. **UTF-8 编码**：所有字符串默认使用 UTF-8 编码
3. **字面量类型为 `literal_view`**：字符串字面量 `"hello"` 的类型是 `std.literal_view`
4. **零成本抽象**：通过 Static Mode 实现字面量的零拷贝初始化
5. **显式分配**：明确区分视图（切片）和拥有所有权的字符串对象

### 5.2 string 类

```cpp
module std.string;

public class string {
    private byte^ data;
    private size len;
    private size cap;
    
    // 内部标志位（存储在 capacity 的高位）
    private const FLAG_SSO = 0x0000; // 小字符串优化
    private const FLAG_HEAP = 0x4000; // 堆分配
    private const FLAG_STATIC = 0x8000; // 静态模式（指向 .rodata）
    
    // 构造
    public string();
    public string(string& other);
    public string(string~ other);
    public string(literal_view lv); // 零拷贝构造（Static Mode）
    public string(string_view sv);
    public string(byte^ cstr);
    public string(byte^ str, size n);
    public string(size n, byte c);
    
    public ~;
    
    // 赋值
    public func operator=(self, string& other) -> string&;
    public func operator=(self, string~ other) -> string&;
    public func operator=(self, literal_view lv) -> string&;
    public func operator=(self, string_view sv) -> string&;
    public func operator=(self, byte^ cstr) -> string&;
    
    // 元素访问
    public func operator[](self, size index) -> byte&;
    public func at(self, size index) -> byte&;
    public func front(self) -> byte&;
    public func back(self) -> byte&;
    public func c_str(self) -> byte^;
    public func data(self) -> byte^;
    
    // 容量
    public func empty(self) -> bool;
    public func size(self) -> size;
    public func length(self) -> size;
    public func capacity(self) -> size;
    public func reserve(self, size n);
    public func shrink_to_fit(self);
    
    // 修改
    public func clear(self);
    public func insert(self, size pos, byte^ str);
    public func insert(self, size pos, string_view sv);
    public func erase(self, size pos, size len = npos);
    public func push_back(self, byte c);
    public func pop_back(self);
    public func append(self, byte^ str) -> string&;
    public func append(self, string_view sv) -> string&;
    public func operator+=(self, byte^ str) -> string&;
    public func operator+=(self, string_view sv) -> string&;
    public func replace(self, size pos, size len, byte^ str);
    public func replace(self, size pos, size len, string_view sv);
    public func swap(self, string& other);
    
    // 查找
    public func find(self, byte^ str, size pos = 0) -> size;
    public func find(self, string_view sv, size pos = 0) -> size;
    public func find(self, byte c, size pos = 0) -> size;
    public func rfind(self, byte^ str, size pos = npos) -> size;
    public func rfind(self, string_view sv, size pos = npos) -> size;
    public func rfind(self, byte c, size pos = npos) -> size;
    
    // 比较
    public func compare(self, string_view other) -> int;
    public func compare(self, byte^ str) -> int;
    
    // 子串
    public func substr(self, size pos = 0, size len = npos) -> string;
    
    // UTF-8 支持
    public func count(self) -> size; // 返回字符数量（解码 UTF-8）
    
    // 静态常量
    public static const size npos = size.max;
}
```

### 5.3 string_view 类

```cpp
// string_view 是 byte![] 的类型别名
// 这是一个非拥有的 UTF-8 字符串切片
public using string_view = byte![];
```

### 5.4 字符串操作

```cpp
// 字符串连接
public func operator+(string_view lhs, string_view rhs) -> string;
public func operator+(string_view lhs, byte^ rhs) -> string;
public func operator+(byte^ lhs, string_view rhs) -> string;

// 字符串比较
public func operator==(string_view lhs, string_view rhs) -> bool;
public func operator==(string_view lhs, byte^ rhs) -> bool;
public func operator!=(string_view lhs, string_view rhs) -> bool;
public func operator!=(string_view lhs, byte^ rhs) -> bool;
public func operator<(string_view lhs, string_view rhs) -> bool;
public func operator<(string_view lhs, byte^ rhs) -> bool;

// 类型转换
public func to_string(int value) -> string;
public func to_string(long value) -> string;
public func to_string(float value) -> string;
public func to_string(double value) -> string;

public func stoi(string_view str) -> int;
public func stol(string_view str) -> long;
public func stof(string_view str) -> float;
public func stod(string_view str) -> double;
```

### 5.5 字符串字面量前缀

```cpp
// 标准库提供的字符串字面量前缀操作符
// $ 前缀返回 std.string 类型
public static func operator "$"(byte[] raw) -> string {
    return string(raw);
}

// 插值字符串支持
public class InterpolatedStringHandler {
    private string buffer;
    
    public InterpolatedStringHandler(usize len, usize count) {
        buffer.reserve(len);
    }
    
    public func append_literal(string_view s) {
        buffer.append(s);
    }
    
    public func append_formatted<T>(T val) {
        buffer.append(to_string(val));
    }
    
    public func get_result(self) -> string {
        return buffer;
    }
}

public static func operator "$"(usize len, usize count) -> InterpolatedStringHandler {
    return InterpolatedStringHandler(len, count);
}
```

## 6. 容器模块（std.container）

### 6.1 设计说明

C^ 的容器系统基于以下核心设计：

1. **显式分配**：明确区分栈数组、堆数组和切片
2. **切片优先**：使用 `Type[]` 切片作为通用的视图类型
3. **零成本抽象**：容器操作在零开销的前提下提供便利性
4. **类型安全**：通过类型系统防止越界访问等错误

### 6.2 动态数组（vector）

```cpp
module std.container;

public class vector<T> {
    private T^ data;
    private size sz;
    private size cap;
    
    // 构造
    public vector();
    public vector(size n);
    public vector(size n, T& value);
    public vector(vector& other);
    public vector(vector~ other);
    public vector(T[] items); // 从切片构造
    
    public ~vector();
    
    // 赋值
    public func operator=(self, vector& other) -> vector&;
    public func operator=(self, vector~ other) -> vector&;
    
    // 元素访问
    public func operator[](self, size index) -> T&;
    public func at(self, size index) -> T&;
    public func front(self) -> T&;
    public func back(self) -> T&;
    public func data(self) -> T^;
    
    // 容量
    public func empty(self) -> bool;
    public func size(self) -> size;
    public func capacity(self) -> size;
    public func reserve(self, size n);
    public func shrink_to_fit(self);
    
    // 修改
    public func clear(self);
    public func insert(self, size pos, T& value);
    public func insert(self, size pos, T~ value);
    public func erase(self, size pos);
    public func erase(self, size first, size last);
    public func push_back(self, T& value);
    public func push_back(self, T~ value);
    public func pop_back(self);
    public func resize(self, size n);
    public func resize(self, size n, T& value);
    public func swap(self, vector& other);
}
```

### 6.3 数组类型

C^ 支持多种数组类型，每种都有明确的语义：

#### 6.3.1 固定大小数组（栈数组）

```cpp
// 固定大小数组在栈上分配，大小在编译期确定
int[5] arr1 = [1, 2, 3, 4, 5];

// 使用 [$] 自动推导栈数组大小
int[$] arr2 = [1, 2, 3]; // 推导为 int[3]
```

#### 6.3.2 切片（动态视图）

```cpp
// 切片是胖指针，包含指针和长度
// 字面量默认推导为切片
var s1 = [1, 2, 3]; // 类型为 int![]（只读切片）

// 可变切片
int[] s2 = arr1; // 从固定数组创建切片
```

#### 6.3.3 动态数组（堆分配）

```cpp
// 堆分配的动态数组，返回切片
int[] heap_arr = new int[5];

// 释放动态数组
delete heap_arr; // 编译器自动识别切片长度
```

#### 6.3.4 多维数组

```cpp
// 矩形数组（连续存储）
int[2, 3] matrix = [
    [1, 2, 3],
    [4, 5, 6]
];

// 交错数组（数组的数组）
int[][] jagged = [
    [1, 2, 3],
    [4, 5]
];
```

### 6.4 链表（list）

```cpp
public class list<T> {
    private Node^ head;
    private Node^ tail;
    private size sz;
    
    public list();
    public list(size n);
    public list(size n, T& value);
    
    // 类似 vector 的接口
    public func empty(self) -> bool;
    public func size(self) -> size;
    public func front(self) -> T&;
    public func back(self) -> T&;
    
    public func push_front(self, T& value);
    public func push_front(self, T~ value);
    public func pop_front(self);
    public func push_back(self, T& value);
    public func push_back(self, T~ value);
    public func pop_back(self);
    
    public func insert(self, iterator pos, T& value);
    public func erase(self, iterator pos);
    public func clear(self);
    public func swap(self, list& other);
}
```

### 6.5 映射（hash_map）

```cpp
public class hash_map<K, V> where K : Hashable {
    private Bucket^ buckets;
    private size bucket_count;
    private size sz;
    
    public hash_map();
    public hash_map(size n);
    
    // 元素访问
    public func operator[](self, K& key) -> V&;
    public func at(self, K& key) -> V&;
    
    // 容量
    public func empty(self) -> bool;
    public func size(self) -> size;
    
    // 修改
    public func insert(self, K& key, V& value);
    public func insert(self, K& key, V~ value);
    public func erase(self, K& key);
    public func clear(self);
    public func swap(self, hash_map& other);
    
    // 查找
    public func find(self, K& key) -> iterator;
    public func contains(self, K& key) -> bool;
    public func count(self, K& key) -> size;
}
```

## 7. 算法模块（std.algorithm）

### 7.1 设计说明

C^ 的算法模块基于以下核心设计：

1. **切片优先**：所有算法都接受切片类型 `T[]` 作为参数
2. **泛型支持**：使用概念（concept）约束算法的适用类型
3. **零成本抽象**：算法在零开销的前提下提供便利性
4. **函数式风格**：支持函数式编程风格的算法组合

### 7.2 非修改式算法

```cpp
module std.algorithm;

// 查找
public func find<T>(T[] data, T& value) -> size;
public func find_if<T>(T[] data, func bool(T&) pred) -> size;
public func find_if_not<T>(T[] data, func bool(T&) pred) -> size;

// 计数
public func count<T>(T[] data, T& value) -> size;
public func count_if<T>(T[] data, func bool(T&) pred) -> size;

// 比较
public func equal<T>(T[] data1, T[] data2) -> bool;
public func mismatch<T>(T[] data1, T[] data2) -> (size, size);

// 搜索
public func search<T>(T[] data1, T[] data2) -> size;
public func find_end<T>(T[] data1, T[] data2) -> size;
```

### 7.3 修改式算法

```cpp
// 复制
public func copy<T>(T[] src, T[] dst) -> size;
public func copy_if<T>(T[] src, T[] dst, func bool(T&) pred) -> size;

// 填充
public func fill<T>(T[] data, T& value);
public func fill_n<T>(T[] data, size n, T& value);

// 生成
public func generate<T>(T[] data, func T() gen);

// 替换
public func replace<T>(T[] data, T& old_val, T& new_val);
public func replace_if<T>(T[] data, func bool(T&) pred, T& new_val);

// 删除（逻辑删除，返回新结尾）
public func remove<T>(T[] data, T& value) -> size;
public func remove_if<T>(T[] data, func bool(T&) pred) -> size;

// 唯一化
public func unique<T>(T[] data) -> size;
```

### 7.4 排序算法

```cpp
// 排序
public func sort<T>(T[] data) where T : Comparable;
public func sort<T>(T[] data, func bool(T&, T&) comp);

// 部分排序
public func partial_sort<T>(T[] data, size middle);

// 堆操作
public func make_heap<T>(T[] data);
public func push_heap<T>(T[] data);
public func pop_heap<T>(T[] data);
public func sort_heap<T>(T[] data);

// 二分查找（要求已排序）
public func binary_search<T>(T[] data, T& value) -> bool;
public func lower_bound<T>(T[] data, T& value) -> size;
public func upper_bound<T>(T[] data, T& value) -> size;
```

## 8. 数学模块（std.math）

### 8.1 基本数学函数

```cpp
module std.math;

// 常量
public const double PI = 3.14159265358979323846;
public const double E = 2.71828182845904523536;

// 三角函数
public func sin(double x) -> double;
public func cos(double x) -> double;
public func tan(double x) -> double;
public func asin(double x) -> double;
public func acos(double x) -> double;
public func atan(double x) -> double;
public func atan2(double y, double x) -> double;

// 双曲函数
public func sinh(double x) -> double;
public func cosh(double x) -> double;
public func tanh(double x) -> double;

// 指数和对数
public func exp(double x) -> double;
public func log(double x) -> double;    // 自然对数
public func log10(double x) -> double;  // 常用对数
public func log2(double x) -> double;   // 二进制对数

// 幂函数
public func pow(double base, double exp) -> double;
public func sqrt(double x) -> double;
public func cbrt(double x) -> double;

// 取整
public func ceil(double x) -> double;
public func floor(double x) -> double;
public func round(double x) -> double;
public func trunc(double x) -> double;

// 绝对值
public func abs(int x) -> int;
public func abs(long x) -> long;
public func abs(float x) -> float;
public func fabs(double x) -> double;

// 最值
public func fmin(double x, double y) -> double;
public func fmax(double x, double y) -> double;

// 浮点操作
public func modf(double x, double^ intpart) -> double;
public func frexp(double x, int^ exp) -> double;
public func ldexp(double x, int exp) -> double;
```

## 9. IO 模块（std.io）

### 9.1 输出

```cpp
module std.io;

// 标准输出
public func print(string_view str);
public func print(byte^ str);
public func println(string_view str);
public func println(byte^ str);

// 格式化输出（使用插值字符串）
// var name = "Alice";
// var age = 18;
// println($"Name: {name}, Age: {age}");

// 标准流
public var stdout: FileStream;
public var stderr: FileStream;
```

### 9.2 文件操作

```cpp
public class File {
    private int fd;
    
    public static func open(string_view path) -> File;
    public static func create(string_view path) -> File;
    
    public func read(self, byte^ buffer, size n) -> size;
    public func write(self, byte^ buffer, size n) -> size;
    public func close(self);
    
    public func seek(self, long offset);
    public func tell(self) -> long;
    
    public func readAll(self) -> string;
    public func writeAll(self, string_view str);
}
```

## 10. 元组模块（std.tuple）

### 10.1 设计说明

C^ 的元组是内置的一等公民类型，提供零开销的匿名数据结构：

1. **值类型**：元组在栈上分配，没有堆分配
2. **解构支持**：支持模式匹配解构
3. **多返回值**：轻松实现多返回值
4. **命名元组**：支持带字段名的元组

### 10.2 元组类型

```cpp
module std.tuple;

// 元组是语言内置类型，标准库提供辅助函数

// 获取元组元素
public func get<T...>(tuple& t, size index) -> T;

// 元组大小
public func size<T...>(tuple& t) -> size;

// 元组连接
public func concat<T1..., T2...>(tuple<T1...> t1, tuple<T2...> t2) -> tuple<T1..., T2...>;

// 元组转换
public func to_tuple<T...>(T[] arr) -> tuple<T...>;
```

## 11. 正则表达式模块（std.regex）

### 11.1 regex 类

```cpp
module std.regex;

public class regex {
    private void^ compiled_pattern;
    
    // 构造函数
    public regex(string_view pattern);
    
    // 匹配
    public func match(self, string_view text) -> bool;
    public func search(self, string_view text) -> match_result;
    
    // 替换
    public func replace(self, string_view text, string_view replacement) -> string;
    
    public ~;
}

// 字符串前缀操作符
public static func operator "regex"(string_view pattern) -> regex {
    return regex(pattern);
}

public class match_result {
    private void^ internal_match;

    public match_result();
    public func success(self) -> bool;
    public func groups(self) -> size;
    public func operator[](self, size index) -> string_view; // 获取捕获组
    public ~;
}
