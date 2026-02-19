# C^ 语言调用 C 库设计文档

## 1. 背景与问题

C^ 语言需要调用 C 库和系统 API 来实现控制台输入输出等基础功能。当前项目中已经有 `extern "C"` 的语法，但需要明确：

- `extern "C"` 和 `[dllimport]` 是否重复？
- 哪种方式更优雅？

## 2. 核心概念分析

### 2.1 `extern "C"` 的作用

`extern "C"` 主要解决两个问题：

1. **链接约定（Linkage Convention）**：指定使用 C 语言的函数调用约定和名字修饰规则
   - C 语言不进行名字修饰（name mangling）
   - C++ 会进行名字修饰（包含参数类型等信息）

2. **ABI 兼容性**：确保与 C 编译的库二进制兼容

### 2.2 `[dllimport]` 的作用

`[dllimport]`（或类似语法）主要用于：

- 从动态链接库（DLL）导入函数
- 告诉编译器该函数在外部 DLL 中
- 生成正确的导入代码（使用 `__imp_` 前缀）

### 2.3 两者是否重复？

**结论：不重复，它们解决的是不同层面的问题：**

| 特性 | 作用层面 | 解决的问题 |
|------|---------|-----------|
| `extern "C"` | 语言层面 | 链接约定、名字修饰、ABI 兼容性 |
| `[dllimport]` | 平台层面 | DLL 导入、动态链接 |

## 3. 设计方案

### 3.1 推荐方案：统一使用 `extern "C"`

**设计原则：**

1. **`extern "C"` 是必须的**：用于 C ABI 兼容性
2. **DLL 导入由编译器自动处理**：根据平台和链接方式自动判断
3. **保持语法简洁**：避免用户需要关心平台细节

### 3.2 语法设计

```cpp
// 方式 1：extern 块（推荐，用于多个函数）
extern "C" {
    func puts(byte^ s) -> int;
    func printf(byte^ format, ...) -> int;
    func malloc(int size) -> byte^;
    func free(byte^ ptr);
}

// 方式 2：单个 extern 函数
extern "C" func GetStdHandle(long nStdHandle) -> void^;

// 方式 3：指定库名（可选）
extern "C" lib "kernel32.dll" {
    func GetStdHandle(long nStdHandle) -> void^;
    func WriteFile(void^ hFile, byte^ lpBuffer, int nNumberOfBytesToWrite, int^ lpNumberOfBytesWritten, void^ lpOverlapped) -> int;
}
```

### 3.3 编译器实现策略

编译器根据以下信息自动处理：

1. **Windows 平台**：
   - 如果指定了 `lib "xxx.dll"`，生成 DLL 导入代码
   - 否则，假设是静态链接或自动从导入库查找

2. **Unix/Linux 平台**：
   - `[dllimport]` 不适用，忽略
   - 依赖链接器处理动态链接

## 4. 标准库实现示例

### 4.1 控制台 I/O 模块

```cpp
// std/io/Console.ch
module std.io.Console;

extern "C" {
    func puts(byte^ s) -> int;
    func printf(byte^ format, ...) -> int;
    func getchar() -> int;
    func putchar(int c) -> int;
}

public func println(string message) -> void {
    puts(message.ptr);
    putchar('\n');
}

public func print(string message) -> void {
    puts(message.ptr);
}

public func readLine() -> string {
    // 实现...
    return "";
}
```

### 4.2 Windows API 模块

```cpp
// std/sys/Windows.ch
module std.sys.Windows;

extern "C" lib "kernel32.dll" {
    func GetStdHandle(long nStdHandle) -> void^;
    func WriteFile(void^ hFile, byte^ lpBuffer, int nNumberOfBytesToWrite, int^ lpNumberOfBytesWritten, void^ lpOverlapped) -> int;
    func ReadFile(void^ hFile, byte^ lpBuffer, int nNumberOfBytesToRead, int^ lpNumberOfBytesRead, void^ lpOverlapped) -> int;
}

public const long STD_INPUT_HANDLE = -10;
public const long STD_OUTPUT_HANDLE = -11;
public const long STD_ERROR_HANDLE = -12;
```

## 5. 使用示例

### 5.1 简单的 Hello World

```cpp
module main;

import std.io.Console;

func main() -> int {
    Console.println("Hello, World!");
    return 0;
}
```

### 5.2 直接使用 C 函数

```cpp
module main;

extern "C" {
    func puts(byte^ s) -> int;
}

func main() -> int {
    puts("Hello from C!");
    return 0;
}
```

## 6. 总结

**推荐方案：**
1. **统一使用 `extern "C"`** 来声明 C 函数
2. **可选使用 `lib "xxx.dll"`** 来指定库名
3. **编译器自动处理** DLL 导入等平台细节
4. **不引入 `[dllimport]`** 特性，避免重复和复杂性

**优势：**
- 语法简洁，用户无需关心平台细节
- 跨平台兼容性好
- 与现有的 C/C++ 生态系统无缝集成
- 保持语言设计的一致性
