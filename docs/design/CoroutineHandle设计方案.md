# CoroutineHandle 设计方案文档

## 1. 问题背景

在 C^ 的协程设计中，`CoroutineHandle` 是一个编译器内置类型，用于表示和操作协程。然而，作为内置类型，用户无法看到其具体定义，这可能导致以下问题：

1. **接口不透明**：用户不清楚 `CoroutineHandle` 提供了哪些方法
2. **使用困惑**：用户可能不知道如何正确使用协程句柄
3. **类型安全**：缺乏明确的接口约束可能导致运行时错误

## 2. 设计方案比较

### 2.1 方案一：属性标记方案

使用属性标记来标识协程相关类型和方法：

```c^
[coroutine]
struct CoroutineHandle {
    [resume]
    func resume();
    
    [destroy]
    func destroy();
    
    [done]
    func done() -> bool;
    
    [promise]
    func promise<T>() -> T;
};
```

**优点**：
- 明确标识协程相关的类型和方法
- 编译时可以进行更严格的类型检查
- 与现有的属性系统保持一致

**缺点**：
- 增加了语法复杂性
- 需要额外的属性解析逻辑
- 与 C^ 的简洁设计理念有些冲突

### 2.2 方案二：鸭子类型方案

基于 C^ 现有的契约式设计理念，提供鸭子类型接口：

```c^
// 鸭子类型接口（概念性）
interface CoroutineHandleConcept {
    func resume();
    func destroy();
    func done() -> bool;
    func promise<T>() -> T;
}

// 编译器内置实现
builtin struct CoroutineHandle {
    static func from_promise<T>(T promise) -> CoroutineHandle;
    func resume();
    func destroy();
    func done() -> bool;
    func promise<T>() -> T;
}
```

**优点**：
- 符合 C^ 现有的契约式设计理念
- 更灵活，用户可以自定义实现
- 不需要特殊的语法支持
- 与现有的 Promise/Awaitable 契约保持一致

**缺点**：
- 类型检查可能不够严格
- 用户可能会实现不符合预期的接口

## 3. 推荐方案

基于 C^ 的设计哲学，**推荐使用鸭子类型方案**，理由如下：

1. **符合设计哲学**：C^ 的协程设计已经采用了契约式（Duck Typing）模型，Promise 和 Awaitable 都是基于契约的
2. **零依赖原则**：鸭子类型方案不需要额外的语法特性，保持了语言的简洁性
3. **灵活性**：用户可以根据需要实现自己的协程句柄类型，只要满足接口契约
4. **一致性**：与现有的协程契约设计保持一致，降低学习成本

## 4. 实现细节

### 4.1 编译器内置类型

```c^
// 编译器内置类型，表示协程的句柄
builtin struct CoroutineHandle {
    // 从 Promise 创建句柄
    static func from_promise<T>(T promise) -> CoroutineHandle;
    
    // 恢复协程执行
    func resume();
    
    // 销毁协程
    func destroy();
    
    // 检查协程是否完成
    func done() -> bool;
    
    // 获取 Promise 对象
    func promise<T>() -> T;
}
```

### 4.2 鸭子类型契约

编译器会检查任何用作协程句柄的类型是否满足以下契约：

```c^
// 协程句柄契约
struct CoroutineHandleLike {
    // 必须提供 resume 方法
    func resume();
    
    // 必须提供 destroy 方法
    func destroy();
    
    // 必须提供 done 方法
    func done() -> bool;
    
    // 必须提供 promise 方法
    func promise<T>() -> T;
}
```

### 4.3 使用示例

```c^
// 标准使用方式
struct Generator {
    struct Promise {
        int current_value;

        func get_return_object() -> Generator {
            return Generator(CoroutineHandle.from_promise(self));
        }

        // ... 其他 Promise 方法
    }

    CoroutineHandle handle;

    func next() -> int? {
        if (handle.done()) return null;
        handle.resume();
        if (handle.done()) return null;
        return handle.promise<Promise>().current_value;
    }
    
    func ~Generator() {
        if (handle) handle.destroy();
    }
}

// 自定义协程句柄实现（符合鸭子类型契约）
struct CustomCoroutineHandle {
    void* frame;
    
    func resume() {
        // 自定义实现
    }
    
    func destroy() {
        // 自定义实现
    }
    
    func done() -> bool {
        // 自定义实现
        return false;
    }
    
    func promise<T>() -> T {
        // 自定义实现
        return *reinterpret_cast<T*>(frame);
    }
}

// 使用自定义句柄
struct CustomGenerator {
    struct Promise {
        int current_value;

        func get_return_object() -> CustomGenerator {
            return CustomGenerator(CustomCoroutineHandle{/* 初始化 */});
        }

        // ... 其他 Promise 方法
    }

    CustomCoroutineHandle handle;

    func next() -> int? {
        if (handle.done()) return null;
        handle.resume();
        if (handle.done()) return null;
        return handle.promise<Promise>().current_value;
    }
    
    func ~CustomGenerator() {
        handle.destroy();
    }
}
```

## 5. 编译器验证

编译器会在以下场景进行验证：

1. **Promise 契约检查**：当一个类型被用作协程返回类型时，编译器会检查其内嵌的 `Promise` 类型是否满足 Promise 契约
2. **Awaitable 契约检查**：当一个表达式被 `await` 时，编译器会检查其类型是否满足 Awaitable 契约
3. **CoroutineHandle 契约检查**：当一个类型被用作协程句柄时，编译器会检查其是否满足 CoroutineHandle 契约

## 6. 设计优势

1. **统一性**：CoroutineHandle 契约与 Promise、Awaitable 契约保持一致的设计风格
2. **灵活性**：用户可以自定义协程句柄实现，满足特殊需求
3. **类型安全**：编译期检查确保所有协程相关类型都满足相应契约
4. **零依赖**：核心机制完全内置于编译器，不依赖标准库
5. **易于理解**：基于鸭子类型的设计符合 C^ 的整体设计哲学

## 7. 总结

通过采用鸭子类型方案，C^ 的协程系统保持了设计的一致性和灵活性：

- **保持简洁**：不需要额外的语法特性
- **符合哲学**：延续了契约式编程模型
- **增强表达力**：允许用户自定义协程句柄实现
- **类型安全**：编译期契约检查确保正确性

这种设计方案既解决了 CoroutineHandle 作为内置类型的接口透明性问题，又保持了 C^ 语言的简洁性和一致性。