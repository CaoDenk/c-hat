# C^ (C-Hat) 编译器

C^ 是一门面向系统编程的实验性语言，目标是结合：

- C/C++ 的底层能力
- Rust 的显式资源与安全方向
- 现代语言的模块、泛型、错误处理和元编程

本仓库是 C^ 的编译器实现，使用 C++23 + LLVM 21。

> **当前状态**
>
> 设计正在收敛，**不是稳定语言版本**。
> 设计决策以 `docs/choose/` 下的 ADR 和
> [`docs/spec/C^ 语言实现现状与语法速查.md`](<docs/spec/C^ 语言实现现状与语法速查.md>) 为准。
> 本文只描述当前可用的构建方式和语言方向，不把设计稿当作已实现能力。

---

## 1. 当前阶段

项目当前处于 **设计冻结 + 核心实现** 阶段：

- ADR 008–020 已收敛核心方向；
- SemanticModel 已引入，变量类型开始优先使用语义类型；
- parser/semantic 覆盖度高于 codegen；
- 基础函数、变量、控制流、类、结构体、数组/切片、元组、泛型、异常、defer 等已有端到端能力；
- 高级特性（虚表、自动析构、模块独立编译、标准库、反射、comptime、协程）仍在实现或标记 experimental。

设计入口：

- [设计冻结清单](docs/design/设计冻结清单.md)
- [ADR 008：类型与代码生成边界](docs/choose/008_类型与代码生成边界.md)
- [ADR 009：内存与资源模型](docs/choose/009_内存与资源模型.md)
- [ADR 010：OOP 派发模型](docs/choose/010_OOP派发模型.md)
- [ADR 011：错误处理模型](docs/choose/011_错误处理模型.md)
- [ADR 012：未初始化变量与 late 语义](docs/choose/012_未初始化变量与late语义.md)
- [ADR 013：语法 canonical form](<docs/choose/013_语法canonical form.md>)
- [ADR 014：模块 ABI 与标准库](docs/choose/014_模块ABI与标准库.md)
- [ADR 015：编译器运行时与标准库边界](docs/choose/015_编译器运行时与标准库边界.md)
- [ADR 016：数组/切片/元组值语义](docs/choose/016_数组切片与元组值语义.md)
- [ADR 017：泛型与 concept 模型](docs/choose/017_泛型与concept模型.md)
- [ADR 018：FFI 与 ABI 细节](docs/choose/018_FFI与ABI细节.md)
- [ADR 019：反射/comptime/协程范围](docs/choose/019_反射comptime与协程范围.md)
- [ADR 020：工具链/构建/包管理](docs/choose/020_工具链构建与包管理.md)

---

## 2. 快速开始

### 依赖

Ubuntu/Debian：

```bash
sudo apt install cmake llvm-21-dev liblld-21-dev libpolly-21-dev \
                 libzstd-dev libcatch2-dev libargparse-dev
```

### 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

内存较小的环境建议：

```bash
cmake --build build -j1
```

### 运行示例

```bash
./build/src/c_hat_compiler examples/hello.ch --run
./build/src/c_hat_compiler examples/exception.ch --run
./build/src/c_hat_compiler examples/generic_class.ch --run
```

### 测试

```bash
cd build
ctest --output-on-failure
```

当前结果：

- `ctest`：43/43 通过
- `tests/integration`：32 个端到端用例
- `examples/*.ch`：10 个示例全部通过

> 单元测试主要覆盖 lexer/parser/semantic；不要把“单测通过”等同于“端到端可用”。

---

## 3. 语言设计亮点

### 3.1 `self` / `base`

C^ 不使用 `this` / `super`：

```c^
class Dog : Animal {
    Dog(int age) : base(age) { }

    func speak() -> int {
        return base.speak() + 1;
    }
}
```

- `self`：当前实例
- `self!`：只读接收者
- `self~`：移动接收者（experimental）
- `base`：基类子对象
- `base(...)`：基类构造函数调用

### 3.2 显式副作用与所有权

- `&x`：可变借用
- `x`：只读借用
- `^x`：取地址 / 指针
- `x~`：移动（experimental）
- `T![]` / `T[]`：只读切片 / 可变切片
- `literalview`：非拥有字节视图

只读只能“加”，不能“去”：

```c^
int[3] a = [1, 2, 3];
int[] s = a;       // OK
int![] r = s;      // OK
int[] bad = r;     // Error
```

### 3.3 数组 / 切片 / 元组

设计目标：

- 固定数组 `T[N]` 是值类型
- 切片 `T[]` 是视图 `{T*, long len}`
- `.len` 类型为 `long`
- `a..b` 半开区间
- `a..=b` 闭区间
- 元组是值类型

```c^
int[3] a = [1, 2, 3];
int[] s = a[0..2];      // 半开
int[] t = a[0..=2];     // 闭区间
var len = s.len;        // long
```

### 3.4 `defer` 与 RAII

设计目标：

- `defer` 是块级作用域
- 支持 `defer { ... }`
- 栈上/`late` 对象自动析构
- 堆对象显式 `new/delete`
- 异常展开执行 defer + 析构

```c^
func copy() {
    var src = open_read("a");
    defer { src.close(); }

    var dst = open_write("b");
    defer { dst.close(); }
}
```

### 3.5 未初始化变量与 `late`

非 `late` 局部变量必须初始化：

```c^
int x = 0;      // OK
int y;          // Error
late int z;     // OK：预留存储，暂不构造
z = 42;         // 第一次赋值 = 构造
```

### 3.6 错误处理

三层模型：

- `T?`：可能没有值
- `Result<T, E>`：可恢复错误
- `try/catch/throw`：真正异常

异常是语言机制，不依赖标准库类型。没有标准库时仍可抛/捕获用户自定义类型。

### 3.7 泛型与 concept

```c^
func max<T>(T a, T b) -> T where Comparable<T> {
    return a > b ? a : b;
}
```

- 单态化
- `Box<int>` / `Point<int>`
- concept + `requires`
- 简单关联类型
- v1 不承诺类型擦除、运行时泛型反射

---

## 4. 当前可用能力

### 端到端可用

- 函数、变量、控制流
- 基础表达式
- 固定数组 / 切片基础操作
- 元组下标、解构、参数/返回
- 类、字段、方法、属性、static
- 单继承字段布局、`base(...)`、`base.member`、`override`
- 显式 `delete` 调用析构
- `try/catch/throw`、`catch (...)`、类型匹配
- `defer`（当前实现仍为函数级，设计目标为块级）
- 泛型函数/类/结构体基础
- concept 基础
- `extern "C"` 基础调用

### experimental / 未稳定

- 虚表 / 虚方法动态派发
- 接口动态派发
- 自动析构 / 完整 RAII 展开
- 多实现继承
- `self~` / 移动语义
- 反射 / `Fold` / `Expansion`
- comptime 执行
- 协程 `await/yield`
- 字符串插值
- `as` 安全转换
- 默认参数 / 命名参数
- 零开销异常
- 变参 FFI / C 回调
- 标准库分层
- 模块独立对象文件

### 当前限制

- `string` 仍在从编译器内建类型迁移到 `std.string`
- 异常仍是 `setjmp/longjmp`，不是零开销表驱动
- 结构体按值异常不完整
- 自动析构、块级 defer 尚未落地
- 数组字面量内部表示仍在向“固定数组默认”收敛
- 模块 import 尚未完整合并公开符号
- 标准库只有最小 `stdlib/std`

---

## 5. 编译器架构

```text
Lexer
  -> Parser
  -> AST
  -> SemanticAnalyzer + SemanticModel
  -> TypeLowering
  -> LLVMCodeGenerator
  -> LLVM IR / Object / Executable
```

### 前端

- `src/lexer/`
- `src/parser/`
- `src/ast/`
- `src/types/`
- `src/semantic/`

### 后端

- `src/llvm/`
- `src/codegen/`（旧代码生成框架）

### 运行时 / 标准库

- `libc_hat_rt`：设计中的编译器运行时
- `stdlib/`：默认标准库（后置完善）

---

## 6. 项目结构

```text
.
├── src/
│   ├── lexer/
│   ├── parser/
│   ├── ast/
│   ├── types/
│   ├── semantic/
│   ├── codegen/
│   └── llvm/
├── stdlib/
├── tests/
│   ├── integration/
│   ├── parser/
│   ├── semantic/
│   ├── class_system/
│   ├── exception/
│   ├── array/
│   ├── tuple/
│   └── ...
├── examples/
├── docs/
│   ├── choose/          # ADR
│   ├── design/
│   ├── spec/
│   └── dev/
└── CMakeLists.txt
```

---

## 7. 实现路线

按设计冻结清单：

1. SemanticModel + TypeLowering
2. 数组 / 切片 / 元组值语义
3. 块级 defer + cleanup 栈
4. 非 late 必须初始化 + late definite assignment
5. 栈上/late RAII + 异常展开
6. OOP vtable / 接口派发
7. 模块 / ABI / FFI
8. 编译器运行时与标准库边界
9. 泛型 / concept 完整化
10. 标准库最小分层
11. 高级特性（反射 / comptime / 协程）

标准库放在最后，避免在核心语义和 ABI 未稳定时反复返工。

---

## 8. 贡献与设计原则

- 先改设计，再改实现
- 每个特性必须有规范、语义、codegen、正向/负向测试
- 文档状态必须标注：parser only / semantic only / codegen usable / stable
- 不把 parser/semantic 覆盖当成端到端可用
- 标准库可替换，语言核心不依赖标准库
- 异常、defer、RAII、数组/切片是语言核心能力

---

## 9. 相关文档

- [实现现状与语法速查](<docs/spec/C^ 语言实现现状与语法速查.md>)
- [设计冻结清单](docs/design/设计冻结清单.md)
- [类型与代码生成边界评审](docs/design/设计评审-类型与代码生成边界.md)
- [内存与资源模型评审](docs/design/设计评审-内存与资源模型.md)
- [语言关键字规范](<docs/spec/C^ 语言关键字规范.md>)
- [语言错误处理规范](<docs/spec/C^ 语言错误处理规范.md>)
- [语言内存管理规范](<docs/spec/C^ 语言内存管理规范.md>)
- [语言类与面向对象规范](<docs/spec/C^ 语言类与面向对象规范.md>)
