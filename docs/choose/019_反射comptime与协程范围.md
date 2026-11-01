# 019：反射、comptime 与协程范围

**状态**：草案  
**日期**：2026-11-01  
**范围**：运行时反射、`typeof` / `is`、comptime、Fold/Expansion、协程、实验特性边界  
**相关文档**：

- `docs/spec/C^ 语言反射与元编程规范.md`
- `docs/spec/C^ 语言编译期计算规范.md`
- `docs/design/反射设计.md`
- `docs/design/编译期执行.md`
- `docs/design/协程设计.md`
- `docs/design/设计冻结清单.md`

---

## 1. 背景

C^ 文档中包含大量高级特性：

- 运行时反射 / RTTI
- `typeof` / `is` / meta field
- `comptime` 编译期执行
- `Fold` / `Expansion`
- 协程 `yield` / `await`
- 反射元数据 / `[Reflectable]`

当前实现状态：

- `typeof` / `is` / meta field 只有占位实现；
- `Fold` / `Expansion` codegen 是 not implemented；
- `comptime` 块目前按运行时块执行，不是编译期执行；
- 协程 codegen 不完整，`yield/await` 不能稳定运行；
- 反射 codegen 是 not implemented。

如果这些特性继续以“半实现”状态存在，会污染核心语义和 ABI。需要明确 v1 范围。

---

## 2. 决策

### 2.1 v1 稳定范围不包含高级元编程

v1 稳定范围不包含：

- 运行时反射 / RTTI
- `comptime` 执行
- `Fold` / `Expansion`
- 协程 `yield/await`
- 反射元数据 / `[Reflectable]`
- 字符串插值（已单列）
- `as` 安全转换（已单列）

这些特性可以保留 parser/semantic 入口，但必须标记 experimental。

### 2.2 `typeof` / `is`

- `typeof` / `is` 先作为编译期类型查询。
- `typeof` 返回类型信息的能力后续与反射一起设计。
- `is` 的类型判断必须由 semantic 完成，不能 codegen 恒返回 true。
- 运行时 `is`（带 RTTI）不进入 v1。
- 没有反射时，`typeof` / `is` 只用于编译期可判定场景。

### 2.3 运行时反射

- v1 不提供稳定运行时反射 ABI。
- `[Reflectable]` / `obj.rtti` / 元数据表后续设计。
- 反射不能成为异常、OOP、泛型、标准库的隐式依赖。
- 如果未来引入，必须单独 ADR 定义 ABI、元数据格式、二进制大小影响。

### 2.4 comptime

- `comptime` 的长期目标是编译期执行。
- v1 不承诺 `comptime` 执行。
- 当前 `comptime {}` 按运行时块执行的行为不能作为稳定语义。
- v1 可以：
  - 保留 `comptime` 关键字；
  - 对未实现的 comptime 用法报错；
  - 或只支持编译期常量表达式（如 `const`）。
- 完整 comptime 需要：
  - 编译期解释器/JIT；
  - 编译期内存模型；
  - 编译期错误处理；
  - 与宏/反射/泛型的交互。
- 这些进入后续 ADR。

### 2.5 Fold / Expansion

- `Fold` / `Expansion` 是泛型参数包/可变参数的高级能力。
- v1 不进入稳定范围。
- parser/semantic 可以保留实验入口，但 codegen 不应静默生成错误 IR。
- 未实现时应明确报错，而不是返回 nullptr/占位值。

### 2.6 协程

- v1 不承诺协程 ABI。
- `yield` / `await` 保留语法入口，但标记 experimental。
- 协程状态机、promise、awaitable、调度器需要单独 ADR。
- 没有稳定协程 ABI 前，标准库不得依赖协程。
- codegen 对未实现协程应明确报错。

### 2.7 反射/元编程与核心解耦

- 异常、defer、RAII、数组/切片、OOP、泛型、模块不得依赖反射。
- 标准库可以可选使用反射，但核心标准库不强制依赖。
- 反射元数据不能改变对象布局，除非显式开启。
- 关闭反射时，程序仍可正常编译运行。

### 2.8 实验特性标记

实验特性必须在文档和测试中标记：

- `experimental`
- `parser-only`
- `semantic-only`
- `not stable`

不能继续在“已支持”文档中把占位实现写成可用。

---

## 3. 备选方案

### 方案 A：v1 全量实现反射/comptime/协程

优点：特性完整。  
缺点：实现量巨大，会拖慢核心语义和 ABI 收敛。

结论：不选。

### 方案 B：v1 移除所有高级特性

优点：核心最干净。  
缺点：已有 parser/semantic 工作和语言愿景浪费。

结论：不选。

### 方案 C：保留入口，标记 experimental（推荐）

优点：不阻塞核心，保留未来空间。  
缺点：需要明确文档和错误行为。

结论：选 C。

---

## 4. 对现有代码的影响

主要涉及：

- `src/ast/`：ReflectionExpr、FoldExpr、ExpansionExpr、ComptimeStmt、Yield/Await
- `src/semantic/SemanticAnalyzer.*`
- `src/llvm/LLVMCodeGenerator.*`
- `docs/spec/C^ 语言反射与元编程规范.md`
- `docs/spec/C^ 语言编译期计算规范.md`
- `docs/design/反射设计.md`
- `docs/design/编译期执行.md`
- `docs/design/协程设计.md`
- `tests/reflection/`
- `tests/coroutine/`
- `tests/extra_expressions/`

行为变化：

- 未实现特性明确报错，不静默生成占位 IR；
- 文档标记 experimental；
- 标准库不依赖高级特性；
- 核心测试不再把 parser/semantic-only 当端到端可用。

---

## 5. 迁移计划

### Phase 0：标记 experimental

- 更新文档；
- 在测试中标记 experimental；
- codegen 未实现时报错。

### Phase 1：编译期类型查询

- `is` 由 semantic 判定；
- `typeof` 用于编译期类型查询；
- 运行时 RTTI 不进入 v1。

### Phase 2：comptime 设计

- 编译期解释器/JIT；
- 编译期内存/错误模型；
- 与泛型/反射交互。

### Phase 3：反射

- 元数据格式；
- `[Reflectable]`；
- 运行时 typeinfo；
- ABI/二进制大小。

### Phase 4：协程

- 状态机 ABI；
- promise/awaitable；
- 调度器；
- 标准库集成。

---

## 6. 待评审确认的问题

1. v1 是否完全排除运行时反射？
2. `typeof` / `is` 是否只做编译期类型查询？
3. `comptime` 是否进入 v1，还是 experimental？
4. `comptime {}` 当前运行时行为是否必须移除？
5. Fold/Expansion 是否保留 parser 入口？
6. 协程是否保留语法入口但标记 experimental？
7. 未实现特性是否统一报错？
8. 反射元数据是否允许改变对象布局？
9. 标准库是否允许依赖反射？
10. 高级特性是否影响核心 ABI？

---

## 7. 验收标准

- 文档明确标记 experimental；
- 未实现特性 codegen 报错，不生成占位 IR；
- `is` 不再恒为 true；
- `typeof` 不再返回占位 0；
- comptime 不再被误认为稳定；
- 协程未实现时有明确错误；
- 核心测试不依赖高级特性；
- 标准库核心不依赖反射/comptime/协程。
