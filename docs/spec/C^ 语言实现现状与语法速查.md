# C^ 语言实现现状与语法速查

> 本文以**当前编译器实现**为准，不是未来设计稿。
> 规范/设计文档里的示例如果与本文冲突，以本文和测试代码为准。
> 目的：避免“文档写了一套、parser 认另一套、codegen 只认第三套”。

最后更新：2026-10-31

> **设计评审进行中**：核心设计决策已确认，正在按阶段落地。
> 已确认决策：[设计决策-2026-10-31](../design/设计决策-2026-10-31.md)
> 评审草案：
> - [设计评审：类型与代码生成边界](../design/设计评审-类型与代码生成边界.md)
> - [设计评审：内存与资源模型](../design/设计评审-内存与资源模型.md)
> 未落地前，新增语法/高级特性默认冻结。

---

## 1. 入口与函数

```c^
func add(int a, int b) -> int {
    return a + b;
}

func main() -> int {
    return add(20, 22);
}
```

- 函数声明统一用 `func`，返回类型后置：`-> T`。
- 表达式函数体：`func f(int x) -> int => x * 2;`
- 无返回值时写 `-> void`，或省略返回类型。
- 当前**没有**内建 `print` / `println`。要么 `extern "C"` 调 `puts`/`printf`，要么导入标准库模块。

```c^
extern "C" {
    func puts(byte!^ s) -> int;
}

func main() -> int {
    puts("hello");
    return 0;
}
```

## 2. 类、构造、析构、继承

### 2.1 构造函数与类同名

构造函数**不是** `func init`，而是和类同名。下面两种写法 parser 都接受，推荐第一种：

```c^
class Person {
public:
    string name;
    int age;

    // 推荐：无 func，类名即构造函数
    Person(string n, int a) {
        name = n;
        age = a;
    }

    // 也接受：显式 func + 类名
    func Person() {
        name = "Unknown";
        age = 0;
    }
}
```

- 构造函数不能写返回类型。
- 构造函数可以重载（参数列表不同）。
- `new Person("Alice", 30)` 会查找 `Person` 的构造函数。

### 2.2 析构函数是 `~ClassName()`

```c^
class Resource {
    void^ handle;

public:
    Resource() {
        handle = allocate_resource();
    }

    ~Resource() {
        if (handle != null) {
            free_resource(handle);
            handle = null;
        }
    }
}
```

- 析构函数必须写 `~` + 类名，不能写匿名 `~ { ... }`。
- 析构函数不能有返回类型，也不能有参数。

### 2.3 继承与基类调用

```c^
class Animal {
public:
    string name;
    Animal(string n) { name = n; }
    func speak() -> void { }
}

class Dog : Animal {
public:
    Dog(string n) : base(n) { }   // 基类构造用 base(...)

    override func speak() -> void {
        base.speak();              // 基类成员用 base.
    }
}
```

- 继承写 `: Base`，**不写** `: public Base`。
- 基类构造调用写 `: base(args)`；`super` 已移除。
- 访问基类成员写 `base.member` / `base.method()`。
- 多接口：`class Shape : Drawable, Resizable { ... }`。

### 2.4 成员函数与 `self`

`self` 是隐式注入的，可以省略；也可以显式写 `self` / `self!`：

```c^
class Counter {
    int value;

public:
    func inc() -> void { value += 1; }          // 隐式 self
    func get(self!) -> int => self.value;       // 只读 self
}
```

- `self`：可变接收者（默认）。
- `self!`：只读接收者，不能修改字段。
- `self~` **尚未实现**。
- **`this` 不是 C^ 关键字**：源码中的 `this` 会被当作普通标识符，通常报“未声明标识符”；请统一写 `self`。
- 只读方法也可以写成参数列表后的 `!`：`func length()! -> int`。
- `get` / `set` 是属性关键字，普通方法不要叫 `get` / `set`；用 `get_value` / `set_value`。

## 3. 变量与类型

```c^
var x = 10;          // 可变，类型推导
let y = 20;          // 不可重绑定
const int N = 32;    // 编译期常量
late File file;      // 延迟初始化

int[5] arr = [1, 2, 3, 4, 5];   // 固定数组，字面量用 []
int[] slice = arr;              // 切片
int![] ro = arr;                // 只读切片
int^ ptr = ^x;                  // 非空指针，取地址用 ^
int& ref = x;                   // 引用
string? maybe = null;           // 可空类型（如果解析器已支持该写法）
```

- 数组字面量只支持 `[...]`，**不支持** `{1, 2, 3}` 作为数组字面量。
- `int[5]` 是固定长度数组；`int[]` 是切片；`int![]` 是只读切片。
- 只读只能“加”，不能“去”：`T[] -> T![]` 合法；`T![] -> T[]` 应当被拒绝。
- 字符串字面量的类型是 `literalview`；它可以转成 `byte!^` / `byte![]`，**不能**转成可写 `byte^` / `byte[]`。

## 4. 堆内存

```c^
int^ p = new int(42);
Point^ pt = new Point(1.0, 2.0);
int[] arr = new int[10];

delete p;
delete[] arr;
```

- 单对象 `new T(...)` 返回 `T^`。
- 数组 `new T[n]` 返回 `T[]`。
- `delete[]` 已在 parser/AST 中支持；建议数组统一写 `delete[]`。

## 5. 控制流

### 5.1 if / while / for

```c^
if (x > 0) { ... } else { ... }

while (running) { ... }

for (int i = 0; i < 10; i++) { ... }
```

### 5.2 foreach

```c^
foreach (var item : items) {
    // ...
}

foreach (var i : 0..10) {
    // ...
}
```

- 当前关键字是 `foreach`，分隔符是 `:`。
- **不是** `for (var x : ...)`，也**不是** `for (var x in ...)`。
- `foreach` 的 `a..b` 当前按**闭区间**处理（包含 `b`）。
- 切片 `arr[a..b]` 仍按**半开区间**处理（不包含 `b`）。两者暂未统一，写代码时注意。

### 5.3 match

match 当前是**语句**，不是表达式：

```c^
match (code) {
    200 => { puts("OK"); }
    404 => { puts("Not Found"); }
    _   => { puts("Unknown"); }
}
```

- 模式直接写，不加 `case`。
- 默认分支是 `_`，不是 `default`。
- 支持整数字面量、字符串字面量、布尔/字符/null、标识符绑定、`_`、守卫 `if`。
- 分支体是 `{ ... }` 或一条以 `;` 结尾的语句。
- 结构解构（`Point(x, y)`）、`|` 多模式、穷尽性检查、作为表达式返回值尚未实现。

## 6. 泛型

```c^
func max<T>(T a, T b) -> T where Comparable<T> {
    return a > b ? a : b;
}

class Box<T> {
    T value;
}

func main() -> int {
    return max<int>(1, 2);
}
```

- 类型参数写在**函数名/类名之后**：`func foo<T>`、`class Box<T>`。
- **不是** `func<T> foo(...)`。
- 关联类型用 `T.Item`，不是 `T::Item`。
- 泛型函数/类在调用点做单态化：`id<int>` 生成 `id__int`，`Box<int>` 生成 `Box__int`。
- 支持显式类型实参；也支持简单推导（`id(42)`、`first(a)` 从 `T[]` 推导元素类型）。
- `concept X<T> = requires (T a, T b) { { a + b } -> T; };` 已能解析并参与检查。
- 泛型结构体也支持单态化：`struct Point<T>` → `Point__int`，字段/方法可用。
- 类声明上的 `where` 约束会在实例化时检查，例如 `class Box<T> where Comparable<T>`。
- 结构体值构造 `Point<int>(1, 2)` 已可用；泛型类仍建议使用 `new Box<int>(...)`。
- 限制：
  - concept 检查目前覆盖常见内置概念和 requires 中的二元运算/成员方法；关联类型、嵌套 requires、复杂表达式还不完整。
  - 推导只做简单的一一对应；不根据返回类型/上下文推导。

## 7. 模块与 FFI

```c^
module app.main;

import std.io.Console;
public import std.core.Types;

extern "C" {
    func puts(byte!^ s) -> int;
}
```

- 模块声明：`module a.b.c;`
- 导入：`import a.b.c;`，别名 `import a.b.c as x;`
- 导出：`public import ...`
- C 互操作：`extern "C" { ... }`

## 8. 已实现 / 未实现（重要）

> 下面的“端到端”指 `--run` 或“生成 .o → 链接 → 运行”实际跑通，不只是 parser/semantic 单元测试通过。

### 端到端可用（2026-10-31 实测）

- 函数定义/调用（整数参数）、`return`
- 变量声明、算术/比较/逻辑表达式
- 数组字面量 `[...]`、固定数组/切片下标读写
- 数组/切片属性 `.len` / `.ptr`、切片区间 `a[start..end]`（本次修复）
- `new T[n]` 返回切片、`delete[]` 释放底层数组（本次修复）
- 元组下标 `t[N]`、解构、作为函数参数/返回值（本次修复）
- `if/else`
- `while` / `for` / `do-while`（`do-while` 本次修复）
- `goto` / label（本次修复）
- 枚举成员访问 `Color.Green` / `Color::Green`（本次修复）
- `+=` / `-=` / `*=` / `++` / `--`（本次修复）
- `extern "C"` 调用 libc（如 `puts`）
- `new T(args)` 会把构造参数传给构造函数（本次修复）
- 类方法调用 `p.m()` / `p->m()`、裸字段名与 `self.x` 访问（本次修复）
- 属性 getter/setter：`public get x -> T` / `public set x(value: T)`（本次修复）
- 单继承：基类字段布局、`: base(...)`、`override`、继承方法调用（本次修复；静态派发）
- `static` 成员变量与静态方法：`Counter.count` / `Counter.method()`（本次修复）
- 显式 `delete` 调用析构函数（本次修复）
- 结构体字段赋值/读取 `p.x = 7; return p.x;`（本次修复）
- `match`：整数/字符/布尔/字符串、`_` 默认、`if` 守卫、标识符绑定（本次修复）
- `foreach`：数组、切片、`a..b` 范围、可选的 index 变量（本次修复）
- `try/catch/throw`：基础抛出/捕获、跨函数抛出、嵌套 try、`catch (...)`、按类型顺序匹配 catch、未匹配继续向外抛（本版为 setjmp/longjmp 实现）
- `defer`：正常返回/throw 时按 LIFO 执行；含 defer 的中间函数也会在异常穿过时执行清理
- 泛型函数单态化：`id<int>` / `max_of(40, 42)` / `first(a)` / 递归泛型
- 泛型类单态化：`Box<int>` / `Box<double>`（字段、构造、方法、类级 `where`）
- 泛型结构体单态化：`Point<int>` / `Pair<int, double>`（字段、方法、值构造 `Point<int>(...)`）
- concept 检查：`Addable` / `Comparable` 等内置概念，以及自定义 `requires` 的基础能力检查

### 只有 parser/semantic 覆盖，codegen 仍不完整

| 功能 | 现状 |
|---|---|
| `defer` | 能解析并执行；但按函数级收集，不是块级作用域（循环内 defer 只执行一次） |
| `comptime { ... }` 块 | 解析失败（当前只支持部分 comptime 语句） |
| 反射 `Fold` / `Expansion` | codegen 仍是 “not implemented” |
| 协程 `await/yield` | 只有部分语法/分析 |

### 数组 / 元组 / 控制流当前边界

- 多维数组的**嵌套字面量初始化**（如 `[[1,2],[3,4]]`）尚不完整；无初始化器的多维数组下标可用。
- 切片没有运行时越界检查；`a[start..end]` 按半开区间处理。
- 元组比较、命名元组字段访问、元组整体赋值仍未完整实现。
- `new T[n]` 只分配原始内存，不会逐个调用元素构造/析构函数。
- 枚举可用于 `Color.Green` / `Color::Green` 和整数比较，但 `match` 的枚举模式、穷尽性检查尚未实现。

### OOP 当前边界

- 继承目前是**静态派发**：`Animal^ a = new Dog(...); a->speak()` 调用 `Animal_speak`，不会走 `Dog_speak`；虚表/接口动态派发尚未实现。
- 派生类构造函数支持 `: base(...)`（当前支持 0 或 1 个参数）；基类字段会展开到派生类布局中。
- `static` 字段初始化目前只支持字面量；静态方法不能访问实例成员（语义层会检查）。
- 自动析构/RAII 仍未实现；显式 `delete` 已能调用 `ClassName_~ClassName`。
- 多继承、接口作为参数类型/动态派发、属性与继承的复杂交互仍不完整。

### 异常实现限制（v1）

- 当前用 `setjmp` / `longjmp` 传播异常，不是文档里描述的零开销表驱动模型。
- `catch (T e)` 已按类型标签精确匹配：按书写顺序尝试 catch，未匹配的异常继续向外传播；`catch (...)` 接受所有异常。当前不做基类/隐式数值转换匹配（例如 `throw 1` 不会命中 `catch (long e)`）。
- 含 `defer` 的函数会安装函数级清理帧。throw 所在函数以及异常穿过的、含 defer 的中间函数会执行已注册的 defer（LIFO）；运行时 flag 避免执行未进入分支的 defer。
- `defer` 仍按函数级收集，不是精确块作用域：循环内注册的 defer 在函数退出时只执行一次；同函数 try 捕获异常时，defer 可能在 throw 点提前执行。
- 自动析构/RAII 尚未实现；目前只有显式 `delete` 会调用析构函数。
- 异常值按 64 位字传递，适合 int/char/bool/指针/类指针；结构体按值异常尚未完整支持。
- 嵌套 try / `catch (...)` / catch 内再次 throw 已可用。

### 单元测试覆盖

现有单元测试主要验证 lexer/parser/semantic/AST 形状；`tests/integration` 已有 31 个 JIT/端到端用例（函数、循环、类、结构体、match、foreach、异常、catch 类型匹配、defer 展开、数组/切片、元组、do-while/goto/enum、属性/继承/static/析构）。不要把“单测通过”等同于“端到端可用”。

### 文档里出现过、但当前实现不支持

| 写法 | 现状 / 建议 |
|---|---|
| `func init(...)` 作为构造函数 | 改为 `ClassName(...)` 或 `func ClassName(...)` |
| `base.init(...)` | 基类构造写 `: base(...)`；基类成员写 `base.x` |
| `~ { ... }` 匿名析构 | 改为 `~ClassName()` |
| `: public Base` | 改为 `: Base` |
| `func<T> foo(...)` | 改为 `func foo<T>(...)` |
| `T::Item` | 改为 `T.Item` |
| `case x => ...` / `default => ...` | 改为 `x => ...` / `_ => ...` |
| `for (var x in xs)` / `for (var x : xs)` | 改为 `foreach (var x : xs)` |
| 数组字面量 `{1, 2, 3}` | 改为 `[1, 2, 3]` |
| `$"Hello {name}"` 字符串插值 | 尚未实现 |
| `x as T` 安全转换 | 尚未实现 |
| `self~` 移动接收者 | 尚未实现 |
| `obj.@[field]` | 当前文档/实现使用 `obj.[field]` |
| 运行期反射 `[Reflectable]` + `obj.rtti` | 只有设计，未完整实现 |
| C 预处理器 `#define` | 不是 C^ 语法 |

## 9. Linux 构建

依赖（Ubuntu/Debian）：

```bash
sudo apt install cmake llvm-21-dev liblld-21-dev libpolly-21-dev \
                 libzstd-dev libcatch2-dev libargparse-dev
```

配置与构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

产物：

- 编译器：`build/src/c_hat_compiler`
- 测试：`build/tests/**/**_catch2_test`

运行：

```bash
./build/src/c_hat_compiler hello.ch --run          # JIT，直接跑
./build/src/c_hat_compiler hello.ch                # 生成 hello.o，再链接成 hello
./build/src/c_hat_compiler hello.ch --emit-llvm    # 生成 hello.ll
./build/src/c_hat_compiler hello.ch --emit-asm     # 生成 hello.s
./build/src/c_hat_compiler hello.ch --dump-ir --no-verify  # 调试非法 IR
```

平台差异：

- Windows：对象文件 `.obj`，可执行文件 `.exe`，默认 `lld-link` / `link.exe`。
- Linux/macOS：对象文件 `.o`，可执行文件无扩展名，默认用 `$CXX` 或 `c++` 链接。
- Linux 上生成的当前是非 PIC 目标文件，链接时自动加 `-no-pie`。
