# 012：未初始化变量与 `late` 语义

**状态**：草案  
**日期**：2026-11-01  
**范围**：`late`、局部变量初始化、definite assignment、构造/析构、异常展开  
**相关文档**：

- `docs/design/late设计.md`
- `docs/spec/C^ 语言内存管理规范.md`
- `docs/choose/009_内存与资源模型.md`

---

## 1. 背景

C^ 需要一种明确表示“预留存储但暂不初始化”的语法，避免普通变量和未初始化变量混在一起。  
`late` 是这个入口，但仅凭语法不足以保证安全，必须配套初始化证明和析构规则。

---

## 2. 决策

### 2.1 非 `late` 局部变量必须初始化

所有非 `late` 的局部变量，声明时必须有初始化器：

```c^
func main() -> int {
    int x = 0;        // OK
    var y = 42;       // OK
    int z;            // Error: non-late variable must be initialized
    late int w;       // OK：明确表示预留内存、暂不初始化
    return x + y;
}
```

这意味着：

- `var x;` 不允许
- `let x;` 不允许
- `const int N;` 仍然不合法
- `int x = 0;` 是最常见写法

### 2.2 `late` 是唯一未初始化局部存储

```c^
late int x;   // 只预留存储，不初始化
late var y;   // 类型推导 + 未初始化
```

`late` 的作用不是“少写初始化值”，而是明确告诉编译器：

> 这个变量可能在后续某个路径上才构造，编译器必须证明使用前已初始化。

### 2.3 适用范围

这条规则主要针对**局部变量**。

- 函数参数：由调用方初始化，天然合法
- `for` / `foreach` 的迭代变量：由循环初始化语句/迭代过程初始化
- `match` 绑定变量：由模式匹配初始化
- `catch (T e)`：由异常 payload 初始化
- 全局变量：建议也必须初始化；如果允许未初始化全局，应单独设计，不混入 `late`
- 类字段：不适用这条规则，字段由构造函数或默认初始化规则处理；`late` 字段不建议支持

### 2.4 `late` 的赋值语义

- 如果变量未初始化：赋值是在预留空间上 **构造**
- 如果变量已初始化：赋值是正常赋值
- 对非平凡类型，重新赋值要先析构旧值，再构造/赋值新值
- 如果构造失败，旧值已析构，变量进入未初始化状态

### 2.5 definite assignment

编译器必须证明：

- 读取 `x` 前所有路径都初始化了 `x`
- `x` 作为函数参数/返回值前已初始化
- `x` 在 `defer` 中使用时，所有能到达该 `defer` 的路径都已初始化

### 2.6 析构规则

- `late` 对象的析构必须受初始化状态保护
- `try` 中初始化失败时，不析构
- `if/else` 只有一分支初始化时，后续使用应报错
- `catch` 可以作为初始化路径之一
- 只有成功构造的对象才参与析构

### 2.7 `late` 与 `defer`

`defer` 可以引用 `late` 变量，但只有在 `defer` 注册时该变量在所有到达路径上已初始化时才允许。

```c^
func test() {
    late int x;
    x = 10;
    defer println(x); // OK：x 在这里已确定初始化
}
```

如果 `defer` 注册在初始化之前，就不能引用 `x`。

### 2.8 v1 限制

为了避免一开始就把复杂度做爆，v1 建议只支持：

- 标量类型
- 简单 struct/class
- 固定数组
- `if/else`、`try/catch`、顺序语句中的 definite assignment

暂不支持或标记 experimental：

- 循环内 `late` 的复杂初始化合并
- `goto` 与 `late`
- `late` 与移动 `~`
- `late` 与数组元素构造/析构
- `late` 与异常强保证

---

## 3. 备选方案

### 方案 A：普通变量允许未初始化

优点：写法更短。  
缺点：`late` 的价值被弱化，未初始化风险重新出现。

结论：不选。

### 方案 B：所有变量都必须初始化，`late` 是唯一例外（推荐）

优点：语义清晰，`late` 的职责明确。  
缺点：部分代码需要写 `late`。

结论：选 B。

---

## 4. 对现有代码的影响

主要涉及：

- `src/parser/Parser.cpp`
- `src/semantic/SemanticAnalyzer.*`
- `src/llvm/LLVMCodeGenerator.*`
- `tests/late/`
- `tests/integration/IntegrationTest.cpp`
- `docs/spec/C^ 语言内存管理规范.md`
- `docs/spec/C^ 语言实现现状与语法速查.md`

行为变化：

- `var x;` / `let x;` 等未初始化局部声明将报错
- `late` 的 definite assignment 检查必须覆盖 `if/else`、`try/catch`、`return`
- `late` 只在成功构造后析构
- `defer` 中引用 `late` 变量时，必须保证 defer 注册时该变量在所有到达路径上已初始化
- 文档中未实现的 `late` 能力需要更新

---

## 5. 待评审确认的问题

1. 普通局部变量是否必须初始化？
2. `late` 是否允许 `late let x;`？
3. `late` 是否允许数组？
4. `late` 是否允许 trivial 类型？
5. `late` 的赋值是构造还是赋值？
6. `late` 的析构只在成功构造后发生？
7. `defer` 中引用 `late` 的条件是什么？
8. v1 是否支持循环内 `late`？
9. v1 是否支持 `goto` 与 `late`？
10. `late` 是否允许在 `catch` 中初始化？

---

## 6. 验收标准

- 非 `late` 局部变量未初始化时 semantic 报错
- `late` 变量使用前 definite assignment 检查
- `late` 对象只在成功构造后析构
- `try/catch` 中 `late` 初始化有测试
- `if/else` 中 `late` 初始化有测试
- `defer` 引用 `late` 有明确规则和测试
- 文档中 `late` 语义与实现一致
