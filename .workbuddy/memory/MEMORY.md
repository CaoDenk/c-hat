# C^ (C-Hat) 项目记忆

## 项目概述
- 语言名：C^ (C-Hat)，文件扩展名 `.ch`
- 目标：性能不低于C++，具备现代安全性，无GC，零成本抽象，全栈能力
- 编译器后端：LLVM，用 C++23 实现
- 构建：CMake + Catch2 测试框架

## 语言核心特性摘要
- 变量：`var`(可变推导) / `let`(不可变推导) / `const`(编译期) / `late`(延迟初始化)，**无 `lazy` 关键字**
- 函数：`func` 关键字，后置返回类型 `-> Type`，支持多返回值元组
- 指针：`^` 符号（取代`*`），`^x` 取地址，`p^` 解引用，空安全 `Type?^`
- 引用：`&` 符号，可变引用调用处必须显式 `&x`，不可变引用 `!&` 可隐式传递
- 不可变标记：`!` 后缀，如 `func foo()!` 或 `self!` 参数
- 移动语义：`~` 后缀
- 数组/切片：`Type[N]` 固定数组，`Type[]` 切片视图，`Type![]` 只读切片，`Type[$]` 通用语法糖（编译器推导长度）
- 模式匹配：`match` 替代 switch，强制穷举；范围语法 `m..n` 为**左闭右开 `[m,n)`**，不支持 `..=`
- 错误处理：异常(try-catch，无finally) + Result<T,E> + Optional<T> + `?` 传播操作符（作用于 `T?`/`Result<T,E>`，其他类型编译错误）；`!` 后缀强制解包（失败 panic 带文件行号，非可空类型使用编译错误）
- 泛型：`<T> where Concept<T>`，Concepts约束，支持万能引用 `T&~`
- 模块：`module A.B;` 声明，移除 namespace，`public` 即导出
- 属性：分离式 `get/set` 关键字语法，`public get int Id => id_;` / `public set Id(int value) => id_ = value;`
- 继承：单继承+多接口，接口支持默认实现；**`extension` 只加方法，不能让类型满足新接口**，接口只能在类定义时声明
- 构造：同类名，栈 `A a("x");`，堆 `A^ p = new A("x");`，无参括号可省略（`A a;` / `new A`）
- 析构/内存：完全对齐 C++，栈自动析构，堆手动 `delete`，标准库提供 `UniquePtr<T>`/`SharedPtr<T>`
- 协程：鸭子类型，`await`/`yield` 关键字（去掉 `co_` 前缀）；Promise 查找**只看返回类型内嵌的 `Promise` 类型**，不支持外部挂载（无 `coroutine_traits`）；`await` 结果类型由 `on_resume()` 返回类型静态决定；静态访问用 `.` 不用 `::`
- 元组解构：同 C#，`var (a, b) = get_pair();`
- 函数类型：`func(int)->int`（薄指针/闭包统一写法），`->void` 可省略写成 `func(int)`；捕获语法同 C++（`[]`/`[&]`/`[=]`/混合/移动捕获）
- Lambda：`[](int x) => x+1`，箭头简写 `n => n > 2` 默认 `[=]` 捕获，递归用 `self` 参数
- `late`：编译期栈空间预留，不调用构造函数，配合CFG追踪初始化状态
- 协程：`await`/`yield` 关键字（待实现）
- 静态反射：`@T` 获取编译期元对象，`comptime for (var field : @T.fields)` 展开，`obj.[field]` 字段访问，`field.type == typeof(int)` 类型比较；元数据属性全用内置类型（`literalview`/`bool`/`int`/`long`），**零标准库依赖**；`[Reflectable]` opt-in 运行时 RTTI（仅注入 `rtti.name`）；`extension` 只加方法不实现接口
- 宏：`#if`/`#define` 预处理器风格，条件编译用 `#if`，但非必要场合不推荐使用；`comptime if` 是语言级编译期分支，两者职责不同
- 无 `unsafe` 关键字：对标 C++，全程信任用户，所有操作由用户负责
- 并发：完全交给标准库（`Thread`、`Atomic`、`Mutex` 等），语言层面无特殊支持
- 类型集合约束：`using Numeric = int | long | float | double`，`|` 是 Concept 约束的语法糖，只能用于 `using` 右侧或 `where` 子句，**不能作为变量类型或函数参数类型**，无运行时开销

## 内置类型命名规范（2026-03-27 确定）
- **内置类型一律全小写**：`int`、`byte`、`char`、`bool`、`long`、`literalview`（无例外，包括复合名）
- **标准库类型 PascalCase**：`String`、`List<T>` 等
- **用户定义类型 PascalCase**：`class Person`、`struct Point` 等
- `LiteralView` → **`literalview`**（已全文档批量替换，263处+3个文件名）
- `StringView` → **废弃**，统一使用 `literalview`
- **`arr.len` 类型为 `long`**（64位有符号），`$expr` = `len - expr` 类型也是 `long`；不用无符号 `usize` 避免下溢

## 数组字面量默认存储（2026-03-27 确定）
- `var arr = [1,2,3]` → `int![]`，`.rodata`（只读切片，零开销）
- `int[$] arr = [1,2,3]` → `int[3]`，栈（显式可变，`[$]` 是全语言通用语法糖）
- `var s = "hello"` → `literalview`，`.rodata`（编译器内置类型）
- `var t = (1,2)` → `(int,int)`，栈（元组始终是值类型）
- `Type[$]` 语法**保留**

## 开发进度（截至2026-03-10）
- 第一阶段（基础解析）✅
- 第二阶段（语义分析）✅
- 第三阶段（LLVM代码生成）✅（基础完成）
- 第四阶段（优化）📋 待做

## 设计文档位置
- `docs/design/`：61个设计文档
- `docs/spec/`：18个规范文档
- `docs/choose/`：设计决策讨论文档
