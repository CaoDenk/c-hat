# 013：语法 Canonical Form

**状态**：核心决策已确认，实现待落地  
**日期**：2026-11-01  
**范围**：参数顺序、构造函数、`defer`、范围、`self`/`base`、`literalview`/`string`、关键字与符号职责  
**相关文档**：

- `docs/design/设计不足之处.md`
- `docs/spec/C^ 语言语法规范.md`
- `docs/spec/C^ 语言语句规范.md`
- `docs/spec/C^ 语言关键字规范.md`
- `docs/spec/C^ 语言实现现状与语法速查.md`

---

## 1. 背景

C^ 当前存在多套写法并存的问题。部分写法是兼容层，部分是历史遗留，部分只是文档示例。  
如果继续扩展语法，parser、semantic、codegen 和文档会继续分叉。

本 ADR 的目标是：**为每个语法主题定义唯一推荐写法，并把其他写法标为兼容、过渡或不支持。**

---

## 2. 决策

### 2.1 参数声明：类型在前

canonical：

```c^
func add(int a, int b) -> int {
    return a + b;
}
```

- canonical：`Type name`
- `name: Type` 不再作为推荐写法；parser 可保留过渡兼容，但文档和测试只使用 `Type name`
- 不推荐：`func foo<T> foo(...)`
- 不支持：命名参数 `add(b: 22, a: 20)`（v1 不实现）

### 2.2 构造函数：类名即构造函数

canonical：

```c^
class Person {
    Person(string name, int age) {
        self.name = name;
        self.age = age;
    }
}
```

- 推荐：`ClassName(...)`，无 `func`
- 兼容：`func ClassName(...)`
- 不支持：`func init(...)`
- 基类构造：`: base(args)`，空参数 `: base()`

### 2.3 `self` / `base`

- `self`：当前实例
- `self!`：只读接收者
- `self~`：移动接收者（语法保留，语义后续实现）
- `base`：基类子对象
- `base(...)`：基类构造函数调用
- `super` 已移除
- `this` 不是关键字

### 2.4 `defer`

canonical：

```c^
// 单表达式：省略花括号
defer close(file);

// 多语句：必须使用块
defer {
    flush(file);
    close(file);
}
```

- 单表达式 `defer expr;` 是推荐写法
- 多语句必须写 `defer { ... }`
- `defer` 是块级作用域
- `defer expr;` 在作用域退出时执行表达式，不是注册时求值
- 表达式结果丢弃；纯表达式可以给 warning
- `defer` 体内不允许 `return` / `break` / `continue`
- `defer` 内再写 `defer` 先禁止或标记 experimental
- 同一作用域内按 LIFO 执行
- 循环体内每次迭代注册、每次迭代结束执行
- `defer` 不应抛异常；异常展开中再次抛异常时 `terminate`

### 2.5 范围

canonical：

- `a..b`：半开区间 `[a, b)`
- `a..=b`：闭区间 `[a, b]`

因此：

```c^
foreach (var i : 0..4) { }   // 0,1,2,3
foreach (var i : 0..=4) { }  // 0,1,2,3,4
```

- 不再保留 `foreach` 闭区间、切片半开区间不一致
- `arr[a..b]` 半开区间
- `arr[a..=b]` 闭区间
- `.len` 的类型统一为 `long`；切片内部长度字段也使用 `long`
- 索引参数接受整数类型，长度/范围计算使用 `long`

### 2.6 访问修饰符与属性

canonical：

```c^
class Counter {
private:
    int _count;

public:
    get count -> int {
        return _count;
    }

    set count(int value) {
        _count = value;
    }
}
```

- 访问修饰符：`public` / `private` / `protected` / `internal`
- 属性：`get name -> T` / `set name(Type value)`
- `get` / `set` 是属性关键字，普通方法不要叫 `get` / `set`
- 属性 getter/setter 本质上是方法

### 2.7 `match`

`match` 是语句，不是表达式。

```c^
match (code) {
    200 => { puts("OK"); }
    404 => { puts("Not Found"); }
    _   => { puts("Unknown"); }
}
```

- 模式直接写，不加 `case`
- 默认分支是 `_`
- 支持整数、字符、布尔、字符串、标识符绑定、`_`、`if` 守卫
- 结构解构、多模式、穷尽性检查后续实现

### 2.8 `foreach`

canonical：

```c^
foreach (var item : items) { }
foreach (var i : 0..4) { }
```

- 关键字是 `foreach`
- 分隔符是 `:`
- `a..b` 默认半开区间
- 数组 `arr[a..b]` 也是半开区间

### 2.9 泛型与关联类型

canonical：

```c^
func max<T>(T a, T b) -> T where Comparable<T> {
    return a > b ? a : b;
}

class Box<T> {
    T value;
}
```

- 类型参数写在函数名/类名之后：`func foo<T>`、`class Box<T>`
- 不写 `func<T> foo(...)`
- 关联类型用 `T.Item`，不写 `T::Item`

### 2.10 `literalview` 与 `string`

- `literalview` 是编译器内建的非拥有视图
- `string` 属于标准库，是拥有型字符串类型，不是编译器内建类
- 字符串字面量是 `literalview`
- `literalview` 转 `string` 需要显式分配/转换
- `literalview` 可转 `byte!^` / `byte![]`
- `literalview` 不可转可写 `byte^` / `byte[]`
- `string` 的存储、分配、比较、格式化由 `std.string` 提供

### 2.11 只读转换

只读只能“加”，不能“去”：

- `T[] -> T![]` 合法
- `T![] -> T[]` 非法
- `T^ -> T!^` 合法
- `T!^ -> T^` 非法
- `literalview -> byte!^ / byte![]` 合法

### 2.12 `const` / `let` / `var` / `late`

- `var`：可变绑定
- `let`：不可重绑定
- `const`：编译期常量
- `late`：未初始化局部存储，必须 definite assignment

非 `late` 局部变量必须初始化。

### 2.13 `new` / `delete`

canonical：

```c^
int^ p = new int(42);
delete p;

int[] arr = new int[3];
delete arr;
```

- `new T(...)` 返回 `T^`
- `new T[n]` 返回 `T[]`
- `delete` 可用于指针和 owning slice
- `delete[]` 仅作为兼容/底层形式保留

### 2.14 `match` / `foreach` / `try` 语句

- `match` 是语句，不是表达式
- `foreach` 是关键字，不是 `for`
- `try/catch/throw` 是异常处理，不移除
- `defer` 是资源清理，不移除

### 2.15 `as` / 字符串插值

- `$"Hello {name}"`：语法糖，编译器只负责解析和 lowering；实际格式化/拼接由标准库插值 API 完成，不在编译器内硬编码格式化规则
- `x as T`：设计为 checked conversion，失败时走错误处理；位级转换应使用单独的 `unsafe cast`/`bitcast`，不要混用 `as`
- `self~`：语法保留，语义后续实现
- 以上三项在 v1 先标记为 experimental，不进入稳定 canonical form

### 2.16 已确认补充决策

1. **`Type name` 是唯一 canonical 参数形式。**
   - `name: Type` 在 v1 过渡期可解析，但标为 deprecated。
   - 文档、测试、示例只使用 `Type name`。
   - v1 后移除 `name: Type`。

2. **`ClassName(...)` 是唯一 canonical 构造函数形式。**
   - `func ClassName(...)` 在 v1 过渡期兼容。
   - 文档只使用 `ClassName(...)`。

3. **`defer expr;` 与 `defer { ... }` 都进入 v1。**
   - 单表达式省略花括号是推荐写法。
   - 多语句必须使用块。
   - `defer expr;` 在作用域退出时执行表达式，不是注册时求值。

4. **范围直接统一为半开/闭区间。**
   - `a..b` 半开 `[a,b)`。
   - `a..=b` 闭区间 `[a,b]`。
   - 不保留旧 inclusive `..` 模式。

5. **`len` 类型为 `long`。**
   - 切片内部长度字段使用 `long`。
   - 索引接受整数类型，内部按 `long` 计算。
   - `foreach` 范围端点使用 `long`；循环变量默认推导为 `long`，显式类型可写 `int` 但需转换。

6. **`string` 属于 `std.string`。**
   - `literalview` 是编译器内建视图。
   - 字符串字面量是 `literalview`。
   - `string` 的最小 API 在标准库 ADR 中定义，不在编译器内硬编码。

7. **`as` 是 checked conversion，v1 experimental。**
   - 失败时走 nullable/Result 错误处理。
   - 位级转换使用单独的 `unsafe cast`/`bitcast`。

8. **字符串插值 lowering 到标准库协议，v1 experimental。**
   - 默认实现由 `std.text` 提供。
   - 自定义标准库可实现自己的插值协议。

9. **`self~` v1 experimental。**
   - 语法保留，不进入稳定语义。

10. **`::` 只用于命名空间/类型限定。**
    - 基类访问用 `base`。
    - 关联类型用 `T.Item`，不用 `T::Item`。

---

## 3. 对现有代码的影响

主要涉及：

- `src/parser/Parser.cpp`
- `src/lexer/Lexer.cpp`
- `src/semantic/SemanticAnalyzer.*`
- `src/llvm/LLVMCodeGenerator.*`
- `docs/spec/C^ 语言语法规范.md`
- `docs/spec/C^ 语言语句规范.md`
- `docs/spec/C^ 语言关键字规范.md`
- `docs/spec/C^ 语言类型系统规范.md`
- `docs/spec/C^ 语言泛型规范.md`
- `docs/spec/C^ 语言错误处理规范.md`
- `docs/spec/C^ 语言内存管理规范.md`
- `tests/parser/`
- `tests/integration/IntegrationTest.cpp`

行为变化：

- `..` 语义从 inclusive 变为 half-open，需要更新测试和示例
- `defer { ... }` 需要 parser 支持
- 参数 canonical form 需要统一
- 构造函数 canonical form 需要统一
- `literalview` / `string` 需要明确转换规则

---

## 4. 迁移计划

### Phase 0：冻结与记录

- 每个主题只保留一个推荐写法
- 兼容写法明确标记为 deprecated
- 更新文档中的示例
- 增加正向/负向测试

### Phase 1：语法 canonical form

- `Type name` 为 canonical
- `ClassName(...)` 为 canonical
- `defer { ... }` 正式支持
- `..` / `..=` 语义统一
- `self` / `base` 为 canonical

### Phase 2：类型与 ABI

- `literalview` / `string` 转换规则
- 只读转换规则
- 数组 / 切片 / 元组表示
- `len` / `ptr` 属性

### Phase 3：高级语法

- `as`
- 字符串插值
- `self~`
- 默认参数 / 命名参数

---

## 5. 开放问题确认结果

1. `name: Type` 在 v1 过渡期兼容，标 deprecated，v1 后移除。
2. `func ClassName(...)` 过渡期兼容，文档只使用 `ClassName(...)`。
3. `defer expr;` 与 `defer { ... }` 都进入 v1；单表达式省略花括号。
4. `..` 直接统一半开，新增 `..=` 闭区间。
5. `len` 为 `long`；索引内部按 `long`；范围端点/循环变量默认 `long`。
6. `string` 属于 `std.string`，最小 API 在标准库 ADR 定义。
7. `as` 是 checked conversion，v1 experimental。
8. 字符串插值 lowering 到标准库协议，默认 `std.text`。
9. `self~` v1 experimental。
10. `::` 只用于命名空间/类型限定；基类用 `base`。

---

## 6. 验收标准

- 每个语法主题只有一个推荐写法
- 文档中的示例全部使用 canonical form
- 兼容写法明确标记为 deprecated
- `..` / `..=` 语义有端到端测试
- `defer expr;` 与 `defer { ... }` 都有端到端测试
- `Type name` / `name: Type` 的 canonical form 有测试
- `ClassName(...)` / `func ClassName(...)` 的 canonical form 有测试
- `literalview` / `string` 转换规则有测试
- 只读转换规则有测试
- 文档与 parser/semantic/codegen 行为一致
