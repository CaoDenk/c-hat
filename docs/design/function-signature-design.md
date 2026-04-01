# 函数签名设计文档

## 1. 概述

在支持函数重载的编程语言中，编译器需要为每个函数重载生成唯一的内部名称，以便在代码生成阶段正确识别和调用。本文档讨论C^语言中函数签名的设计方案，比较不同方案的优缺点，并说明当前采用的方案。

## 2. 当前实现方案：特殊字符分隔

### 2.1 实现方式

目前C^编译器使用特殊字符（`@` 和 `#`）作为函数名和参数类型的分隔符：

- 函数名与参数类型之间使用 `@` 分隔
- 不同参数类型之间使用 `#` 分隔

例如：

- `add(int, int)` → `add@int#int`
- `add(double, double)` → `add@double#double`
- `add(int, double)` → `add@int#double`
- `foo(string, int)` → `foo@string#int`

### 2.2 命名空间支持

对于命名空间中的函数，使用 `.` 作为命名空间分隔符：

- `namespace Math { func add(int, int) }` → `Math.add@int#int`
- `namespace Utils.Math { func add(int, int) }` → `Utils.Math.add@int#int`

### 2.3 优点

- **避免命名冲突**：特殊字符 `@` 和 `#` 不太可能出现在函数名或类型名中，有效避免了命名冲突
- **分隔符明确**：`@` 明确标识参数类型的开始，`#` 明确分隔不同参数类型
- **易于解析**：可以轻松从生成的名称中提取原始函数名和参数类型

### 2.4 缺点

- **特殊字符限制**：某些平台或工具可能不支持包含 `@` 或 `#` 的符号名
- **可读性稍差**：相比纯字母数字的命名，包含特殊字符的名称可读性稍低

## 3. 其他候选方案

### 3.1 方案1：下划线分割

使用下划线(`_`)作为分隔符：

- `add(int, int)` → `add_int_int`
- `add(double, double)` → `add_double_double`

#### 优点
- 实现简单，易于理解
- 生成的函数名可读性较好
- 与C/C++的命名风格一致

#### 缺点
- **命名冲突风险**：如果函数名本身包含下划线，可能与参数类型的分割混淆
  例如：函数`add_int(int)`会被生成为`add_int_int`，与`add(int, int)`的签名冲突

### 3.2 方案2：双下划线分割

使用双下划线(`__`)作为分隔符：

- `add(int, int)` → `add__int__int`
- `add(double, double)` → `add__double__double`

#### 优点
- 仍然使用下划线，保持与C/C++风格的一致性
- 减少了与函数名中单个下划线的冲突

#### 缺点
- 仍然存在冲突风险，只是概率较低
- 函数名可能变得过长

### 3.3 方案3：数字前缀

为每个函数重载分配唯一的数字ID：

- `add(int, int)` → `add_1`
- `add(double, double)` → `add_2`

#### 优点
- 完全避免了命名冲突
- 生成的函数名简洁

#### 缺点
- 函数名失去了参数类型信息，不利于调试和分析
- 需要维护函数ID的映射表

### 3.4 方案4：类型编码

使用简短的类型编码代替完整类型名：

- `int` → `i`
- `double` → `d`
- `byte` → `b`

生成的函数名：

- `add(int, int)` → `add_ii`
- `add(double, double)` → `add_dd`

#### 优点
- 生成的函数名简洁
- 保留了参数类型的信息

#### 缺点
- 需要维护类型编码表
- 对于复杂类型，编码可能不够直观

## 4. 方案比较

| 方案 | 示例 | 优点 | 缺点 | 当前状态 |
|------|------|------|------|----------|
| 特殊字符分隔 | `add@int#int` | 避免冲突，分隔明确 | 特殊字符可能不被支持 | ✅ **当前采用** |
| 下划线分割 | `add_int_int` | 实现简单，可读性好 | 可能与函数名冲突 | ❌ |
| 双下划线分割 | `add__int__int` | 减少冲突，风格一致 | 仍有冲突风险 | ❌ |
| 数字前缀 | `add_1` | 完全避免冲突 | 失去类型信息 | ❌ |
| 类型编码 | `add_ii` | 简洁，保留类型信息 | 需要维护编码表 | ❌ |

## 5. 实现细节

### 5.1 代码位置

函数签名的生成主要在以下文件中：

- `src/llvm/LLVMCodeGenerator.cpp` 中的 `createFunctionPrototype` 和 `generateFunctionBody` 函数

### 5.2 类型名称映射

当前实现中，类型名称的映射规则如下：

| LLVM 类型 | 生成的类型名 |
|-----------|--------------|
| `i1`, `i8`, `i16`, `i32`, `i64` 等 | `int` |
| `float`, `double` | `double` |
| 指针类型 | `ptr` |
| 结构体类型 | 结构体名称 |
| 其他类型 | `unknown` |

### 5.3 示例代码

```cpp
// 生成唯一函数名
std::string uniqueFuncName = funcDecl->name;
if (!paramTypes.empty()) {
  uniqueFuncName += "@";
  for (size_t i = 0; i < paramTypes.size(); ++i) {
    if (i > 0) {
      uniqueFuncName += "#";
    }
    uniqueFuncName += getTypeName(paramTypes[i]);
  }
}
```

## 6. 未来改进方向

### 6.1 类型名称优化

当前所有整数类型都映射为 `int`，未来可以考虑更精确的类型映射：

- `i8` → `i8` 或 `byte`
- `i16` → `i16` 或 `short`
- `i32` → `i32` 或 `int`
- `i64` → `i64` 或 `long`

### 6.2 复杂类型支持

对于复杂类型（如泛型、数组、切片等），需要设计更完善的类型名称生成规则：

#### 6.2.1 泛型类型

```cpp
// 泛型类型的mangling规则
List<int>        → List@int
List<List<int>>  → List@List@int
Map<string, int> → Map@string#int

// 函数签名示例
func process(List<int> data) → process@List@int
func process(List<List<int>> data) → process@List@List@int
```

**设计原则：**
- 使用 `@` 分隔类型名和泛型参数
- 使用 `#` 分隔多个泛型参数
- 支持嵌套泛型（递归应用规则）

#### 6.2.2 数组与切片类型

```cpp
// 切片类型
int[]       → int[]      // 使用 [] 表示切片
int![]      → int![]     // 保留 ! 修饰符

// 固定大小数组
int[5]      → int[5]     // 保留数组大小
int[$]      → int[$]     // 自动推导标记

// 多维数组
int[,]      → int[,]     // 矩形切片（逗号表示维度）
int[][]     → int[][]    // 交错数组

// 函数签名示例
func sum(int[] data) → sum@int[]
func process(int[5] arr) → process@int[5]
func matrix(int[,] m) → matrix@int[,]
```

#### 6.2.3 指针与引用类型

```cpp
// 指针类型
int^        → int^       // 裸指针
int!^       → int!^      // 指向只读数据的指针

// 引用类型
int&        → int&       // 左值引用
int!&       → int!&      // 只读引用
int&~       → int&~      // 万能引用（仅泛型推导上下文）

// 函数签名示例
func get_ptr(int^ p) → get_ptr@int^
func process(int& ref) → process@int&
```

#### 6.2.4 函数指针类型

```cpp
// 函数指针类型
func(int, int) -> int  →  func@int#int@int

// 函数签名示例
func apply(func(int, int) -> int f) → apply@func@int#int@int
```

#### 6.2.5 元组类型

```cpp
// 元组类型
(int, float)    → (int#float)     // 使用 # 分隔元素类型
(int, string, bool) → (int#string#bool)

// 函数签名示例
func process((int, float) tuple) → process@(int#float)
```

### 6.3 类型名称映射完整表

| C^ 类型 | 生成的类型名 | 说明 |
|---------|-------------|------|
| `int` | `int` | 基础类型 |
| `int!` | `int!` | 只读类型 |
| `int[]` | `int[]` | 切片 |
| `int[5]` | `int[5]` | 固定大小数组 |
| `int[,]` | `int[,]` | 多维切片 |
| `int^` | `int^` | 指针 |
| `int&` | `int&` | 引用 |
| `List<int>` | `List@int` | 泛型 |
| `(int, float)` | `(int#float)` | 元组 |
| `func(int, int) -> int` | `func@int#int@int` | 函数指针 |

### 6.3 Name Mangling

如果需要与外部工具或调试器更好地集成，可以考虑实现标准的 name mangling 方案（如 Itanium C++ ABI）。

## 7. 结论

当前采用的特殊字符分隔方案（`@` 和 `#`）在实际使用中表现良好，有效避免了命名冲突，同时保持了较好的可读性。未来可以根据实际需求进行进一步的优化和改进。
