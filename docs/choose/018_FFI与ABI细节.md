# 018：FFI 与 ABI 细节

**状态**：核心决策已确认；out T& 语义已确认，ABI 实现待落地

**日期**：2026-11-02（初稿 2026-11-01）

**范围**：C ABI、`extern "C"`、变参、函数指针、结构体传参、literalview/string、链接、异常边界

**相关文档**：

- `docs/choose/014_模块ABI与标准库.md`
- `docs/choose/015_编译器运行时与标准库边界.md`
- `docs/spec/C^ 语言标准库规范.md`
- `docs/spec/C^ 语言模块系统规范.md`
- `docs/design/互操作与FFI设计.md`
- `docs/design/调用C库设计.md`

---

## 1. 背景

C^ 需要与 C/C++ 生态互操作。当前已有：

- `extern "C"` 基本调用
- `puts` / `printf` 等 libc 调用
- `--c-lib-path` / `--c-lib-file` / `-l`
- Linux/Windows 链接差异

但以下细节未冻结：

- 基础类型到 C ABI 的映射
- 变参调用约定
- 函数指针
- 结构体按值传参
- `literalview` / `string` 到 C 字符串
- 异常跨越 FFI 边界
- 符号可见性与链接模型
- 平台 ABI 差异

---

## 2. 决策

### 2.1 `extern "C"` 规则

- `extern "C"` 函数使用 C ABI。
- 函数名不 mangle。
- 调用约定为平台默认 C 调用约定。
- `extern "C"` 块内只允许函数声明/定义、必要的类型声明。
- C^ 异常不能跨越 C 函数边界传播。
- C 函数不能抛 C^ 异常。
- C++ 异常也不能跨越 C^ 边界（v1 不支持）。

### 2.2 基础类型映射

| C^ 类型 | C ABI 类型 | 说明 |
|---|---|---|
| `bool` | `_Bool` / `bool` | 按平台 |
| `byte` | `uint8_t` | 无符号 8 位 |
| `sbyte` | `int8_t` | 有符号 8 位 |
| `short` | `int16_t` | |
| `ushort` | `uint16_t` | |
| `int` | `int32_t` | |
| `uint` | `uint32_t` | |
| `long` | `int64_t` | C^ `long` 固定 64 位 |
| `ulong` | `uint64_t` | |
| `float` | `float` | |
| `double` | `double` | |
| `char` | `uint32_t` / 平台宽字符 | C^ `char` 是 32 位，不与 C `char` 直接等同 |
| `T^` | `T*` | 指针 |
| `T!^` | `const T*` | 只读指针 |
| `T&` / `T!&` | `T*` / `const T*` | 引用按指针传递 |
| `out T&` | `T*` | 初始化引用；运行时仍是 `T*`，`out` 是语义契约 |
| `literalview` | `{const uint8_t*, long}` | 不是 NUL 结尾 |
| `T[]` | `{T*, long}` | 不直接映射为 C 数组 |

### 2.3 变参 FFI

- `extern "C"` 支持 `...` 变参声明。
- 调用点按 C 默认实参提升规则处理：
  - `float` -> `double`
  - 小整数 -> `int`（按平台/ABI）
- 变参参数类型由调用者显式提供，C^ 不做隐式格式化。
- `printf` 等函数由标准库/用户声明，编译器不内置格式化。
- 变参调用 ABI 需要平台支持。

### 2.4 函数指针

- C^ 函数指针类型：`func(int, int) -> int`。
- 函数指针在 LLVM 中表示为函数指针。
- `extern "C"` 函数指针使用 C 调用约定。
- 普通 C^ 函数指针使用 C^ 调用约定。
- 回调到 C 时，函数指针必须显式 `extern "C"` 或 ABI 兼容。
- v1 支持基础函数指针调用；闭包/捕获后续。

### 2.5 结构体按值传参

- POD struct 可以按值传给 C，布局按平台 ABI。
- 非平凡 struct（析构/构造/拷贝）不直接按值传给 C。
- 结构体字段对齐/填充由 TypeLowering/ABI 决定。
- v1 先支持简单 POD；复杂 ABI 后续。
- C^ class 默认按指针传递，不按值传给 C。

### 2.6 `literalview` / `string` 与 C 字符串

- `literalview` 不是 NUL 结尾，不能直接当 C 字符串。
- 需要显式转换：
  - `literalview` -> `byte!^` 需要确保 NUL 终止
  - 标准库提供 `cstr` 转换/包装
- `string` 是标准库拥有型字符串，不直接映射为 C 字符串。
- C 字符串传入 C^ 时使用 `byte!^` 或 `literalview` 显式构造。

### 2.7 链接与库

- `-l` / `--library` 链接库。
- `--c-lib-path` 指定库搜索路径。
- `--c-lib-file` 指定 C 库文件。
- Linux/macOS：`.o` + `$CXX`/`c++` 或配置 linker。
- Windows：`.obj` + `lld-link`/`link.exe`。
- 静态库/动态库后续支持。
- 运行时 `libc_hat_rt` 与标准库分别链接。

### 2.8 符号可见性与模块

- `extern "C"` 符号不 mangle。
- 普通 C^ 符号按模块 mangling。
- `public` 导出；`internal`/`private` 不导出。
- 模块间调用通过公开声明；不生成头文件。
- 跨模块 ABI 需要稳定 mangling。

### 2.9 异常边界

- C^ 异常不能跨 C 函数边界。
- C 函数不能抛 C^ 异常。

### 2.10 `out T&` 与 FFI

- `out T&` 的运行时表示是 `T*`，与普通 `T&` 相同。
- `out` 是语义签名的一部分，调用方据此在正常返回后标记实参已初始化。
- 跨模块导出签名应记录 `out`；运行时符号可以保持指针 ABI。
- 不建议允许仅靠 `out` 区分重载：调用点都是 `f(&x)`，无法区分。
- `extern "C"` 函数的 `out T&` 是显式信任契约：C 侧无法被 C^ 检查，调用方必须保证 C 函数真的完成初始化。
- C ABI 不携带 `out` 元数据；需要重载语义时使用不同函数名或 C^ 侧包装。
- `out T!&` 非法；只读引用不能用于初始化。
- C++ 异常不能跨 C^ 边界（v1）。
- 回调函数抛异常时，必须在 C^ 边界内捕获；不能让异常穿过 C 栈帧。
- 如果需要跨 FFI 错误传递，使用错误码/Result，不使用异常。

### 2.10 已确认补充决策

1. **`long` 固定 64 位作为 C ABI 映射。**
   - C^ `long` -> `int64_t`。
   - 平台 `long` 差异通过显式 C 类型别名处理。

2. **`char` 固定 32 位，不与 C `char` 直接映射。**
   - C `char` 用 `byte` / `sbyte`。
   - C 宽字符需显式映射。

3. **变参 FFI 标记 experimental。**
   - 基础 `extern "C"` 调用优先。
   - 变参 ABI 后续完整实现。

4. **基础函数指针进入 v1；C 回调 ABI 标记 experimental。**
   - 函数指针类型可用。
   - 跨 C 回调需要 ABI 兼容。

5. **结构体按值传参只支持 POD。**
   - 非平凡 struct 不直接按值传给 C。
   - 复杂 ABI 后续。

6. **`literalview` 到 C 字符串必须显式转换。**
   - `literalview` 不是 NUL 结尾。
   - 标准库提供 `cstr` 转换/包装。

7. **`string` 不直接映射为 C 字符串。**
   - 需要显式转换。
   - `string` 属于标准库。

8. **异常绝对禁止跨 FFI 边界。**
   - C/C++ 异常不能穿过 C^ 栈帧。
   - C^ 异常不能穿过 C 栈帧。
   - 跨 FFI 错误用错误码/Result。

9. **v1 支持链接已有库，不承诺创建静态/动态库。**
   - `-l` / `--c-lib-path` / `--c-lib-file`。
   - 库创建后续。

10. **Windows 是 v1 源码/构建目标。**
    - 二进制 ABI 完全一致不作为 v1 承诺。
    - Windows 需要独立 ABI 测试。

---

## 3. 当前实现状态

### 已实现

- `extern "C"` 基本声明/调用
- `puts` / `printf` 基础调用
- `--library` / `--c-lib-path` / `--c-lib-file`
- Linux/Windows 链接基础

### 未实现

- 变参调用完整 ABI
- 函数指针 C 互操作
- 结构体按值传参
- `literalview` / `string` 到 C 字符串的标准转换
- 跨平台 ABI 验证
- 异常边界检查
- 静态库/动态库

---

## 4. 对现有代码的影响

主要涉及：

- `src/llvm/LLVMCodeGenerator.*`
- `src/semantic/SemanticAnalyzer.*`
- `src/main.cpp`
- `tests/ffi/`
- `tests/integration/IntegrationTest.cpp`
- `docs/design/互操作与FFI设计.md`
- `docs/design/调用C库设计.md`
- `docs/spec/C^ 语言标准库规范.md`

行为变化：

- 变参调用需要按 C ABI 提升
- 函数指针需要区分 C/C^ 调用约定
- 结构体按值传参限制为 POD
- `literalview` 不再隐式当 C 字符串
- 异常不能跨 FFI 边界
- 符号可见性影响链接

---

## 5. 迁移计划

### Phase 0：冻结 ABI 映射

- 基础类型映射
- 指针/引用映射
- `literalview` 映射
- 结构体 POD 规则
- 异常边界规则

### Phase 1：基础 FFI

- `extern "C"` 调用
- `puts` / `printf` 基础
- 链接参数
- 错误边界

### Phase 2：变参与函数指针

- 变参调用 ABI
- 函数指针调用
- 回调
- 默认实参提升

### Phase 3：结构体与字符串

- POD struct 按值传参
- `literalview` / `string` 转换
- C 字符串包装

### Phase 4：跨平台

- Windows/Linux ABI 测试
- 静态库/动态库
- 调用约定

---

## 6. 开放问题确认结果

1. `long` 固定 64 位作为 C ABI 映射。
2. `char` 固定 32 位，不与 C `char` 直接映射。
3. 变参 FFI 标记 experimental。
4. 基础函数指针进入 v1；C 回调 ABI experimental。
5. 结构体按值传参只支持 POD。
6. `literalview` 到 C 字符串必须显式转换。
7. `string` 不直接映射为 C 字符串。
8. 异常绝对禁止跨 FFI 边界。
9. v1 支持链接已有库，不承诺创建静态/动态库。
10. Windows 是 v1 源码/构建目标，ABI 完全一致不作为承诺。
11. `out T&` 运行时映射为 `T*`，`out` 只进入语义签名。
12. extern C 的 `out T&` 是信任契约，不能做过程内检查。
13. 不允许仅靠 `out` 区分重载。

---

## 7. 验收标准

- `extern "C"` 基础调用有端到端测试
- `out T&` 的 ABI 映射与普通 `T&` 一致
- 跨模块 `out` 签名可检查，extern C `out` 有明确信任边界
- `printf` 变参调用有测试（若进入 v1）
- 函数指针回调有测试（若进入 v1）
- POD struct 按值传参有测试（若进入 v1）
- `literalview` 到 C 字符串转换有测试
- 异常跨 FFI 边界有负向测试
- Windows/Linux 链接至少各有一个测试
- 文档与实现一致
