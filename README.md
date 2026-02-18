# C^ (C-Hat) 编译器开发项目

## 项目简介

C^ (C-Hat) 本项目使用AI辅助开发C^语言的完整编译器实现。

## 核心设计理念

- **性能优先**：不低于 C++ 的性能
- **安全性**：空安全、边界检查、类型安全
- **现代语法**：简洁、清晰、无歧义
- **零成本抽象**：编译期展开、静态多态
- **全栈能力**：从操作系统到高层业务

## 语言特性

### 基础类型

- **整数**：`byte`/`sbyte` (8位), `short`/`ushort` (16位), `int`/`uint` (32位), `long`/`ulong` (64位)
- **字符**：`char` (32位 Unicode 码点), `char16` (UTF-16), `char32` (UTF-32)
- **浮点**：`float`, `double`, `fp16` (半精度), `bf16` (Brain Float)
- **布尔**：`bool`
- **字符串**：`string` (标准库类型，UTF-8), `string_view` (字节切片视图，`byte![]` 别名), `wstring`/`u16string` (UTF-16)

### 变量声明

```cpp
// 显式类型声明
int counter = 0;
byte mask = 0xFF;
char ch = '中';

// 类型推导
var total = 100;     // int, 可变
let limit = 50;      // int, 不可变

// 延迟初始化
late int x;           // 延迟初始化变量
x = 42;

// 常量
const int MAX_RETRIES = 3;
```

### 函数定义

```cpp
// 标准函数定义
func calculate_area(int width, int height) -> int {
    return width * height;
}

// 单表达式函数
func add(int a, int b) -> int => a + b;

// 多返回值
func get_status() -> (int, string) {
    return (200, "OK");
}
```

### 控制流

```cpp
// 条件语句 (必须带大括号)
if (x > 0) {
    print("Positive");
} else {
    print("Non-positive");
}

// Match 表达式 (取代 switch)
string status_text = match (code) {
    200 => "OK",
    404 => "Not Found",
    _   => "Unknown"
};

// 循环
for (int i = 0; i < 10; i++) {
    // ...
}

while (running) {
    // ...
}

do {
    // ...
} while (condition);
```

### 数组与切片

```cpp
// 栈上固定数组（值类型）
int[5] arr1 = [1, 2, 3, 4, 5];

// 数组字面量默认推导为栈数组
var arr2 = [1, 2, 3]; // 类型为 int[3]，在栈上

// 只读切片（显式声明）
int![] const_slice = [1, 2, 3]; // 指向 .rodata

// 切片视图
int[] slice = arr2; // 引用已有数组

// 范围切片
int[] sub = arr2[0..2];
```

### 字符串

```cpp
// 字符串字面量（原生类型为 LiteralView）
var s = "hello"; // 类型为 LiteralView（编译器内置类型）

// 显式声明为只读切片（隐式转换）
byte![] view = "hello"; // 类型为 byte![]（string_view）

// 标准库字符串对象
var str = $"hello"; // 类型为 std::string

// 字符串插值
var name = "Alice";
var msg = $"Hello {name}, you are {30} years old.";

// 原始字符串（无需转义）
var path = #"C:\Windows\System32"#;

// 字符串切片
var sub = view[0..3]; // "hel"（string_view）

// 字符字面量
char c1 = 'A';
char c2 = '中';
char c3 = '😊'; // 32位 Unicode 码点

// 不同编码前缀
byte b = u8'A';
char16 c16 = u16'中';
char32 c32 = u32'😊';
```

### 指针系统

```cpp
// 指针操作
var x = 42;
var ptr = ^x;       // 取地址，指针
ptr^ = 100;         // 解引用

// 成员访问
obj->member;         // 指针成员访问
```

### 类与面向对象

```cpp
class Rectangle {
    int width;
    int height;
    
    func area() -> int {
        return width * height;
    }
}

// 访问修饰符
class Person {
    public string name;      // 公共成员
    private int age;      // 私有成员
    protected int id;     // 受保护成员
}
```

### 内存管理

```cpp
// 分配单个对象
var p = new int;
var p = new int(42);

// 分配数组
var arr = new int[10];

// 释放单个对象
delete p;

// 释放数组
delete[] arr;
```

### 异常处理

```cpp
func test() {
    try {
        // 可能抛出异常的代码
        throw "Something went wrong";
    } catch (string err) {
        // 处理异常
    } catch (...) {
        // 兜底异常处理
    }
}
```

### defer 机制

```cpp
func test() {
    var file = open_file("test.txt");
    defer close_file(file);  // 在函数返回前执行
    
    // ... 使用 file ...
    
    // 函数结束时自动调用 close_file(file)
}
```

### 元组

```cpp
// 元组字面量
var t = (1, "hello", 3.14);

// 多返回值
func get_coords() -> (int, int) {
    return (10, 20);
}

var (x, y) = get_coords();
```

## 编译器架构

### 前端组件

1. **词法分析器 (Lexer)**
   - 识别C^的所有token
   - 处理关键字、操作符、字面量
   - 生成token流

2. **语法分析器 (Parser)**
   - 基于EBNF文法的递归下降解析器
   - 构建抽象语法树(AST)
   - 错误恢复机制
   - ✅ 支持 new/delete 表达式解析

3. **语义分析器 (Semantic Analyzer)**
   - 类型检查
   - 符号表管理
   - 作用域分析
   - ✅ 支持 new/delete 表达式语义分析

### 后端组件

4. **LLVM 代码生成器 (LLVM Code Generator)**
   - 将AST转换为LLVM IR
   - 支持 defer 机制
   - 支持类系统代码生成

5. **优化器 (Optimizer)** (待实现)
   - 常量折叠
   - 死代码消除
   - 循环优化
   - 内联展开

## 开发环境

- **操作系统**: Windows/Linux/macOS
- **编程语言**: C++23
- **构建工具**: CMake 3.20+
- **测试框架**: Catch2
- **版本控制**: Git

## 项目结构

```
.
├── src/
│   ├── lexer/          # 词法分析器
│   ├── parser/         # 语法分析器
│   ├── semantic/       # 语义分析器
│   ├── ast/            # 抽象语法树
│   ├── codegen/        # 代码生成器
│   ├── llvm/           # LLVM代码生成
│   └── types/          # 类型系统
├── docs/
│   ├── design/         # 设计文档
│   ├── spec/          # 规范文档
│   └── dev/           # 开发文档
├── tests/             # 测试用例
│   ├── parser/         # 解析器测试
│   ├── semantic/       # 语义分析测试
│   ├── class_system/   # 类系统测试
│   ├── exception/      # 异常处理测试
│   ├── array/          # 数组/切片测试
│   ├── defer/          # defer机制测试
│   ├── tuple/          # 元组测试
│   ├── types/          # 类型系统测试
│   └── new_delete/     # new/delete测试
└── examples/          # 示例代码
```

## 单元测试

项目使用 Catch2 作为测试框架，所有测试均通过！

### 测试覆盖

| 测试模块         | 断言数  | 状态 |
| ---------------- | ------- | ---- |
| Parser 测试      | 64      | ✅    |
| Semantic 测试    | 29      | ✅    |
| Class 测试       | 11      | ✅    |
| Exception 测试   | 3       | ✅    |
| Array 测试       | 7       | ✅    |
| Defer 测试       | 4       | ✅    |
| Tuple 测试       | 2       | ✅    |
| Type System 测试 | 34      | ✅    |
| New/Delete 测试  | 13      | ✅    |
| **总计**         | **167** | ✅    |

### 构建和运行测试

```bash
# 构建项目
mkdir build && cd build
cmake ..
cmake --build . --config Debug

# 运行测试
tests/parser/Debug/parser_catch2_test.exe
tests/semantic/Debug/semantic_catch2_test.exe
# ... 其他测试
```

## 开发路线图

### 第一阶段：基础功能 ✅
- [x] 词法分析器
- [x] 基础语法分析器
- [x] 变量声明解析（var/let/late/const）
- [x] 函数声明解析
- [x] 表达式解析
- [x] 控制流语句解析（if/for/while/do-while/match）
- [x] 类定义解析
- [x] 结构体定义解析
- [x] 枚举定义解析
- [x] Lambda表达式解析
- [x] 数组/切片解析
- [x] 指针系统解析
- [x] new/delete 表达式解析
- [x] 元组解析
- [x] 异常处理解析（try-catch/throw）
- [x] defer 语句解析

### 第二阶段：语义分析 ✅
- [x] 完整类型系统
- [x] 符号表管理
- [x] 作用域分析
- [x] 类型检查
- [x] 类型推导
- [x] new/delete 表达式语义分析
- [x] 异常处理语义分析
- [x] 类系统语义分析（访问修饰符、构造/析构函数识别）
- [x] defer 机制语义分析

### 第三阶段：代码生成 🚧
- [x] LLVM IR 基础代码生成
- [x] 变量声明和函数代码生成
- [x] 类系统代码生成
- [x] defer 机制完整实现（收集和执行）
- [ ] 异常处理 LLVM 代码生成（invoke/landingpad）
- [ ] 构造函数和析构函数代码生成
- [ ] new/delete 代码生成

### 第四阶段：优化 📋
- [ ] 基础优化
- [ ] 高级优化
- [ ] 性能分析
- [ ] 优化反馈

## 最近更新

### 2026-02-18
- ✅ 完善类系统测试（访问修饰符）
- ✅ 所有 167 个单元测试全部通过！
- ✅ 创建单元测试文档和问题记录文档

