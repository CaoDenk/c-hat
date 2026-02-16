# Defer 关键字与资源清理设计文档

## 1. 核心概念

`defer` 是 C^ 语言中用于**确保代码块在作用域结束时执行**的关键字。它提供了一种轻量级的、非侵入式的资源管理机制，作为 RAII（资源获取即初始化）模式的有力补充。

设计灵感来源于 Go 语言的 `defer` 和 C++ 的 `ScopeGuard` 模式，但在语义上进行了更严格的界定，以适应 C^ 的高性能系统编程需求。

## 2. 设计规范

### 2.1 语法定义

`defer` 后面可以接单条语句或复合语句块。

```ebnf
defer_statement ::= 'defer' statement
                  | 'defer' compound_statement
```

示例：

```cpp
func process() {
    FILE^ f = fopen("data.txt", "r");
    if (f == null) return;
    
    // 注册清理操作：无论函数如何退出（return 或 throw），都会执行 fclose
    defer fclose(f);
    
    // 也可以是代码块
    defer {
        print("Exiting process scope");
    }
    
    // 业务逻辑...
}
```

### 2.2 作用域绑定 (Block Scoping)

与 Go 语言不同（Go 的 `defer` 是函数级作用域），**C^ 的 `defer` 是块级作用域 (Block Scope)**。这意味着 `defer` 语句会在**包含它的最近一层花括号 `}` 结束前**执行。

**设计理由**：
*   **确定性**：资源释放的时机更加精确，避免在长函数中持有资源过久。
*   **一致性**：与 C++ 的析构函数行为保持一致（离开作用域即销毁）。

```cpp
func scope_test() {
    {
        defer print("Inner scope exit");
        print("Inside inner");
    } // 这里打印 "Inner scope exit"
    
    print("Outside inner");
}
```

### 2.3 执行顺序 (LIFO)

当同一作用域内有多个 `defer` 语句时，它们按照**后进先出 (LIFO)** 的顺序执行。这对于处理有依赖关系的资源（如嵌套锁）非常关键。

```cpp
func order_test() {
    defer print("First defer");
    defer print("Second defer");
    
    print("Main body");
}
// 输出顺序：
// 1. Main body
// 2. Second defer
// 3. First defer
```

### 2.4 变量捕获 (Capture by Reference)

`defer` 代码块中的变量捕获采用**引用语义**。它直接访问当前上下文中的局部变量，而不是在注册 `defer` 时进行拷贝。

```cpp
func capture_test() {
    int x = 10;
    
    defer {
        print(x); // 打印的是 defer 执行时的值
    }
    
    x = 20;
} 
// 输出: 20
```

## 3. 典型应用场景

### 3.1 遗留 C API 资源管理

在使用未封装 RAII 的 C 语言 API 时，`defer` 是防止内存泄漏的神器。

```cpp
func legacy_api_usage() {
    void^ ptr = malloc(1024);
    defer free(ptr); // 确保释放
    
    if (check_something()) {
        return; // 自动调用 free(ptr)
    }
    
    do_work(ptr);
}
```

### 3.2 事务回滚 (Transaction Rollback)

处理多步操作时，如果中途失败需要回滚之前的操作。

```cpp
func transfer_money(Account& from, Account& to, int amount) {
    from.lock();
    defer from.unlock(); // 确保解锁
    
    to.lock();
    defer to.unlock();   // 确保解锁
    
    if (from.balance < amount) {
        throw TransactionError("Insufficient funds");
        // 自动解锁 to，然后解锁 from
    }
    
    from.balance -= amount;
    to.balance += amount;
}
```

### 3.3 配对操作

对于 `open/close`, `lock/unlock`, `start/stop` 等成对出现的逻辑，`defer` 能让代码更加紧凑且不易出错。

```cpp
imgui.Begin("Window");
defer imgui.End(); // 保持 Begin 和 End 在视觉上靠近

if (imgui.Button("Click")) {
    // ...
}
```

## 4. 与 RAII 的对比与协作

C^ 是一门强 RAII 语言（支持析构函数），为什么还需要 `defer`？

| 特性 | RAII (析构函数) | Defer |
| :--- | :--- | :--- |
| **定义位置** | 类型定义中 (Class level) | 使用点 (Call site) |
| **主要用途** | 封装通用资源的生命周期 (如 `File`, `Socket`, `Lock`) | 处理一次性的、特定的清理逻辑 |
| **优势** | 自动化程度高，不易遗忘 | 灵活，无需定义新类即可实现清理 |
| **推荐场景** | 核心库、通用组件开发 | 业务逻辑、脚本式代码、调用 C API |

**最佳实践**：
*   对于通用的资源（如文件句柄），优先封装成 RAII 类。
*   对于临时的回调、日志记录或组合多个资源的复杂清理逻辑，使用 `defer`。

## 5. 异常安全性

`defer` 块中的代码**不应抛出异常**。如果 `defer` 是在栈展开（Stack Unwinding）过程中被调用的（即因为另一个异常而退出作用域），此时若 `defer` 内部再次抛出异常，将导致程序强制终止（类似 C++ 的 `std::terminate`）。

编译器会对 `defer` 块进行 `noexcept` 推导检查，并在可能抛出异常时发出警告。
