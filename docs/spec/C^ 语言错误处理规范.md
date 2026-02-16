# C^ 语言错误处理规范

## 1. 概述

C^ 语言提供了多种错误处理机制，包括异常、错误码和可选类型等。C^ 的错误处理哲学是：**区分"可恢复的逻辑结果"与"不可恢复的运行时异常"**。本文档详细描述 C^ 语言中错误处理的语法、API 和最佳实践，与设计文档保持一致。

## 2. 错误处理基础

### 2.1 错误处理概念

- **错误**：程序执行过程中发生的异常情况，如文件不存在、网络连接失败等
- **异常**：程序执行过程中发生的中断事件，需要立即处理
- **错误码**：通过返回值表示错误状态的机制
- **可选类型**：用于表示可能失败的操作，返回值或错误信息

### 2.2 错误处理的目标

- **可靠性**：程序能够正确处理各种错误情况
- **可维护性**：错误处理代码清晰、简洁，易于理解和维护
- **用户体验**：错误信息清晰、准确，便于用户理解和解决问题
- **调试友好**：错误信息包含足够的上下文，便于调试

## 3. 异常处理

### 3.1 异常的基本语法

C^ 语言使用 `try`、`catch` 和 `throw` 关键字进行异常处理。C^ 移除了 `finally` 块，资源清理请使用 `defer` 或 RAII（析构函数）。

```cpp
// 异常处理基本语法
try {
    // 可能抛出异常的代码
    if (error_condition) {
        throw Exception("Error message");
    }
} catch (Exception^ e) {
    // 处理异常
    println("Caught exception: " + e.message);
} catch (NetworkError^ e) {
    // 处理特定类型的异常（引用类型）
    log(e.message);
}
```

### 3.2 异常类型体系

所有可抛出的对象**必须**继承自 `std::Exception` 类。C^ 采用了强制继承体系，而非 Duck Typing 契约。

```cpp
// 基础异常类 (标准库提供)
class Exception {
public:
    virtual string message { get; }
    virtual string stack_trace { get; }
    virtual ~ { }
}

// 用户自定义异常
class NetworkError : public Exception {
public:
    NetworkError(string msg) : base(msg) {}
}

// 运行时类型判断
class DivideByZeroError : public Exception {
public:
    DivideByZeroError(string msg) : base(msg) {}
}
```

### 3.3 noexcept 关键字

C^ 引入了 `noexcept` 关键字，用于标记不会抛出异常的函数。异常规范是函数类型的一部分。

```cpp
// 标记为不抛出异常
func swap(int& a, int& b) -> void noexcept {
    // 实现
}

// 条件 noexcept
func process() -> void noexcept(noexcept(perform())) {
    perform();
}

// noexcept 作为编译期操作符
bool safe = noexcept(perform());
```

### 3.4 抛出异常

使用 `throw` 关键字抛出异常：

```cpp
// 抛出异常
func divide(int a, int b) -> int {
    if (b == 0) {
        throw DivideByZeroError("Division by zero");
    }
    return a / b;
}

// 抛出自定义异常
class InvalidInputException : public Exception {
public:
    int input_value;
    
    func init(string message, int value) {
        base.init(message);
        input_value = value;
    }
}

func process_input(int value) -> void {
    if (value < 0) {
        throw InvalidInputException("Input must be non-negative", value);
    }
}
```

### 3.5 捕获异常

使用 `catch` 关键字捕获异常。C^ 中 `catch` 捕获的通常是指针或引用，避免切片问题。

```cpp
// 捕获异常
try {
    int result = divide(10, 0);
    println("Result: " + result);
} catch (DivideByZeroError^ e) {
    println("Error: " + e.message);
} catch (Exception^ e) {
    println("Unexpected error: " + e.message);
}

// 捕获多个异常类型
try {
    process_input(-1);
} catch (InvalidInputException^ e) {
    println("Invalid input: " + e.message + ", value: " + e.input_value);
} catch (Exception^ e) {
    println("Error: " + e.message);
}
```

### 3.6 异常与性能

C^ 采用基于表的异常处理机制（Table-based Exception Handling）：
- **Happy Path 零开销**：只要不抛出异常，进入 `try` 块没有任何运行时指令开销
- **抛出代价**：只有 `throw` 发生时，运行时才会查找异常表来展开栈

因此，异常应当用于**真正的异常情况**（如 IO 错误、网络中断），而不是用于常规的控制流。



## 4. 异常层次结构

### 4.1 内置异常类型

C^ 语言提供了丰富的内置异常类型，形成层次结构：

```
Exception
├── SystemException
│   ├── ArgumentException
│   │   ├── ArgumentNullException
│   │   └── ArgumentOutOfRangeException
│   ├── ArithmeticException
│   │   └── DivideByZeroError
│   ├── IOException
│   │   ├── FileNotFoundError
│   │   └── EndOfStreamException
│   ├── InvalidOperationException
│   └── NullReferenceException
└── ApplicationException
    └── CustomException
```

### 4.2 常用内置异常

| 异常类型                      | 描述                 |
| ----------------------------- | -------------------- |
| `Exception`                   | 所有异常的基类       |
| `SystemException`             | 系统异常基类         |
| `ArgumentException`           | 参数无效             |
| `ArgumentNullException`       | 参数为 null          |
| `ArgumentOutOfRangeException` | 参数超出有效范围     |
| `ArithmeticException`         | 算术错误             |
| `DivideByZeroError`           | 除数为零             |
| `IOException`                 | I/O 操作错误         |
| `FileNotFoundError`           | 文件未找到           |
| `InvalidOperationException`   | 操作在当前状态下无效 |
| `NullReferenceException`      | 尝试访问 null 引用   |
| `IndexOutOfRangeException`    | 索引超出范围         |
| `OutOfMemoryException`        | 内存不足             |
| `StackOverflowException`      | 栈溢出               |

### 4.3 自定义异常

创建自定义异常类：

```cpp
// 自定义异常 - 使用 C^ 继承语法
class MyException : public Exception {
    int error_code;
    
    func init(string message, int code) {
        base.init(message);
        error_code = code;
    }
    
    func init(string message) {
        base.init(message);
        error_code = 0;
    }
}

// 使用自定义异常
try {
    if (some_error_condition) {
        throw MyException("Custom error occurred", 404);
    }
} catch (MyException^ e) {
    println("Custom error: " + e.message + ", code: " + e.error_code);
} catch (Exception^ e) {
    println("General error: " + e.message);
}
```

## 5. 错误码

### 5.1 基本错误码

错误码是一种传统的错误处理机制，通过返回值表示错误状态：

```cpp
// 使用错误码
func open_file(string path, out FileStream^ file) -> int {
    try {
        file = new FileStream(path, FileMode.Open);
        return 0; // 成功
    } catch (FileNotFoundException^ e) {
        file = null;
        return 1; // 文件未找到
    } catch (IOException^ e) {
        file = null;
        return 2; // I/O 错误
    }
}

// 使用错误码
FileStream^ file;
int result = open_file("file.txt", out file);
if (result == 0) {
    // 成功，使用文件
    file.read_all_text();
    file.close();
} else if (result == 1) {
    println("File not found");
} else if (result == 2) {
    println("I/O error");
} else {
    println("Unknown error");
}
```

### 5.2 错误码枚举

使用枚举定义错误码，提高代码可读性：

```cpp
// 错误码枚举
enum ErrorCode {
    Success = 0,
    FileNotFound = 1,
    IOException = 2,
    InvalidArgument = 3,
    OutOfMemory = 4
}

// 使用错误码枚举
func open_file(string path, out FileStream^ file) -> ErrorCode {
    try {
        file = new FileStream(path, FileMode.Open);
        return ErrorCode.Success;
    } catch (FileNotFoundException^ e) {
        file = null;
        return ErrorCode.FileNotFound;
    } catch (IOException^ e) {
        file = null;
        return ErrorCode.IOException;
    }
}

// 使用错误码枚举
FileStream^ file;
ErrorCode result = open_file("file.txt", out file);
if (result == ErrorCode.Success) {
    // 成功
} else if (result == ErrorCode.FileNotFound) {
    println("File not found");
} else {
    println("Error: " + result);
}
```

### 5.3 错误码与异常的结合

错误码和异常可以结合使用，根据不同场景选择合适的错误处理机制：

```cpp
// 错误码与异常的结合
func process_data(string path) -> ErrorCode {
    try {
        // 打开文件
        FileStream^ file = new FileStream(path, FileMode.Open);
        
        // 读取数据
        string data = file.read_all_text();
        
        // 处理数据
        if (data.length == 0) {
            return ErrorCode.InvalidData;
        }
        
        // 处理成功
        return ErrorCode.Success;
    } catch (FileNotFoundException^ e) {
        return ErrorCode.FileNotFound;
    } catch (IOException^ e) {
        return ErrorCode.IOException;
    } catch (Exception^ e) {
        return ErrorCode.UnknownError;
    }
}
```

## 6. 可选类型 (Optional)

C^ 语言移除了一切特殊的空值语法（如 `?` 后缀、`?^` 指针），统一使用标准库提供的 `Optional<T>` 模板类型来表达"可能不存在"的值。这包括值类型的可选值和指针类型的可空指针。

### 6.1 基本语法

`Optional<T>` 是一个包装类型，语义上类似于 `std::optional` 或 Rust 的 `Option`。

```cpp
// 1. 值类型的可选值
Optional<int> opt_val = 42;
Optional<string> opt_str = "Hello";

// 2. 可空指针 (Optional<T^>)
// 指针 T^ 默认不可为 null，必须使用 Optional<T^> 才能表达可空语义
Optional<MyClass^> opt_ptr = new MyClass();

// 3. 空状态
opt_val = null;      // null 隐式转换为 Optional::None
opt_ptr = null;
```

### 6.2 使用 Optional

由于没有隐式的 null 解引用风险，使用 `Optional` 必须显式检查或解包。

```cpp
func find_user(int id) -> Optional<User^> {
    if (exists(id)) {
        return new User(id);
    }
    return null; // 返回 None
}

// 1. 显式检查
Optional<User^> user = find_user(1);
if (user) {
    // user.value() 返回 User^
    print(user.value()^.name);
} else {
    print("User not found");
}

// 2. 模式匹配 (推荐)
match (find_user(2)) {
    case Some(u) => print(u^.name),
    case None    => print("Not found")
}

// 3. 链式操作 (Map / AndThen)
Optional<string> name = find_user(3)
    .map((u) => u^.name); // 如果 user 存在，提取 name；否则返回 None
```

### 6.3 常用方法

`Optional<T>` 提供了一系列方法来安全地处理数据：

```cpp
Optional<int> val = get_value();

// 获取值，如果为空则 panic
int v1 = val.unwrap(); 

// 获取值，如果为空则使用默认值
int v2 = val.value_or(0);

// 获取值，如果为空则调用函数生成默认值
int v3 = val.value_or_else(() => compute_default());
```

## 7. Result 类型

### 7.1 Result 类型的基本语法

C^ 语言提供了 `Result<T, E>` 类型，用于表示可能失败的操作，返回值或错误信息：

```cpp
// Result 类型
Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Result<int, string>.error("Division by zero");
    }
    return Result<int, string>.ok(a / b);
}

// 使用 Result 类型
Result<int, string> result = divide(10, 2);
if (result.is_ok()) {
    int value = result.unwrap();
    println("Result: " + value);
} else {
    string error = result.unwrap_error();
    println("Error: " + error);
}

// 使用模式匹配
match (result) {
    Result<int, string>.ok(int value) => println("Result: " + value),
    Result<int, string>.error(string error) => println("Error: " + error)
}
```

### 7.2 Result 类型的辅助方法

Result 类型提供了丰富的辅助方法：

```cpp
// Result 类型的辅助方法
Result<int, string> result = divide(10, 2);

// 映射成功值
Result<string, string> string_result = result.map<int, string>((int value) => value.ToString());

// 映射错误值
Result<int, int> int_error_result = result.map_error<string, int>((string error) => 400);

// 绑定
Result<int, string> bind_result = result.and_then<int, string>((int value) => {
    if (value > 0) {
        return Result<int, string>.ok(value);
    }
    return Result<int, string>.error("Result must be positive");
});

// 获取值或默认值
int value = result.unwrap_or(0);

// 获取值或计算默认值
int value_with_default = result.unwrap_or_else((string error) => 0);
```

### 7.3 Result 传播操作符 (?)

C^ 引入了后缀 `?` 操作符，用于简化 Result 的传播。语义：如果表达式结果是 Error，立即从当前函数返回该 Error；如果是 Ok(val)，则解包出 val 继续执行。

```cpp
// Result 传播操作符
func read_config() -> Result<Config, Error> {
    var file = File::open("config.ini")?;
    return file.read_all()?.parse_json()?.to_config();
}

func process_data(string s) -> Result<int, ParseError> {
    int val = parse_int(s)?;  // 如果失败，直接返回错误
    return val * 2;
}
```

### 7.4 契约编程 (require/ensure)

C^ 支持基本的契约编程语法，用于文档化和运行时检查函数的前置/后置条件。

```cpp
func divide(int a, int b) -> int 
    require(b != 0)  // 前置条件
    ensure(result * b == a)  // 后置条件
{
    return a / b;
}
```

## 8. 错误处理策略

### 8.1 选择合适的错误处理机制

不同的错误处理机制适用于不同的场景：

| 错误处理机制 | 适用场景                         | 优点                           | 缺点                                       |
| ------------ | -------------------------------- | ------------------------------ | ------------------------------------------ |
| 异常         | 严重错误，需要立即中断执行       | 代码清晰，不需要检查返回值     | 性能开销，可能掩盖错误路径                 |
| 错误码       | 轻微错误，需要继续执行           | 性能好，明确的错误状态         | 代码冗余，需要检查返回值                   |
| 可选类型     | 可能不存在的值                   | 类型安全，代码清晰             | 只能表示存在或不存在，不能表示具体错误原因 |
| Result 类型  | 可能失败的操作，需要返回错误信息 | 类型安全，同时返回值和错误信息 | 代码稍显冗长                               |

### 8.2 错误处理策略示例

#### 8.2.1 异常处理策略

```cpp
// 异常处理策略
func process_file(string path) -> void {
    // 打开文件 - 严重错误，使用异常
    FileStream^ file = null;
    try {
        file = new FileStream(path, FileMode.Open);
    } catch (FileNotFoundException^ e) {
        throw Exception("Input file not found: " + path, e);
    } catch (IOException^ e) {
        throw Exception("Error opening file: " + path, e);
    }
    
    defer {
        if (file != null) {
            file.close();
        }
    };
    
    // 读取文件内容
    string content = file.read_all_text();
    
    // 处理文件内容 - 业务逻辑错误，使用异常
    if (content.length == 0) {
        throw Exception("File is empty: " + path);
    }
    
    // 处理成功
    println("File processed successfully: " + path);
}
```

#### 8.2.2 错误码处理策略

```cpp
// 错误码处理策略
enum NetworkError {
    Success = 0,
    ConnectionFailed = 1,
    Timeout = 2,
    ServerError = 3
}

func send_request(string url, int timeout_ms) -> NetworkError {
    // 发送网络请求
    try {
        NetworkClient^ client = new NetworkClient();
        client.timeout = timeout_ms;
        
        HttpResponse^ response = client.send(url);
        
        if (response.status_code == 200) {
            return NetworkError.Success;
        } else {
            return NetworkError.ServerError;
        }
    } catch (ConnectionException^ e) {
        return NetworkError.ConnectionFailed;
    } catch (TimeoutException^ e) {
        return NetworkError.Timeout;
    } catch (Exception^ e) {
        return NetworkError.ServerError;
    }
}

// 使用错误码
NetworkError result = send_request("https://example.com", 5000);
switch (result) {
    case NetworkError.Success:
        println("Request succeeded");
        break;
    case NetworkError.ConnectionFailed:
        println("Connection failed");
        break;
    case NetworkError.Timeout:
        println("Request timed out");
        break;
    case NetworkError.ServerError:
        println("Server error");
        break;
}
```

#### 8.2.3 可选类型处理策略

```cpp
// 可选类型处理策略
func find_product(int id) -> Product? {
    // 查找产品
    for (auto product in products) {
        if (product.id == id) {
            return product;
        }
    }
    return null; // 未找到产品
}

func get_product_price(int id) -> double? {
    Product? product = find_product(id);
    return product?.price; // 如果产品不存在，返回 null
}

// 使用可选类型
Product? product = find_product(123);
if (product != null) {
    println("Found product: " + product.name + ", price: " + product.price);
} else {
    println("Product not found");
}

double? price = get_product_price(123);
if (price != null) {
    println("Product price: " + price);
} else {
    println("Product not found or no price");
}
```

#### 8.2.4 Result 类型处理策略

```cpp
// Result 类型处理策略
func parse_int(string input) -> Result<int, string> {
    try {
        int value = int.parse(input);
        return Result<int, string>.ok(value);
    } catch (FormatException^ e) {
        return Result<int, string>.error("Invalid format: " + input);
    } catch (OverflowException^ e) {
        return Result<int, string>.error("Value out of range: " + input);
    }
}

// 使用 Result 类型
Result<int, string> result = parse_int("42");
result.match(
    ok: (int value) => println("Parsed value: " + value),
    error: (string error) => println("Error: " + error)
);

// 链式调用
Result<int, string> chained_result = parse_int("42")
    .and_then((int value) => {
        if (value > 0) {
            return Result<int, string>.ok(value);
        }
        return Result<int, string>.error("Value must be positive");
    })
    .map((int value) => value * 2);

chained_result.match(
    ok: (int value) => println("Final result: " + value),
    error: (string error) => println("Error: " + error)
);
```

## 9. 错误处理的最佳实践

### 9.1 异常处理最佳实践

1. **只在异常情况下使用异常**：异常应该用于真正异常的情况，而不是常规的控制流
2. **抛出具体的异常类型**：使用最具体的异常类型，便于捕获和处理
3. **提供有意义的异常信息**：异常消息应该清晰、准确，包含足够的上下文信息
4. **不要捕获所有异常**：避免使用 `catch (Exception^ e)` 捕获所有异常，应该只捕获预期的异常
5. **使用 defer 或 RAII 释放资源**：确保资源在异常情况下也能正确释放
6. **不要忽略异常**：捕获异常后应该处理它，而不是忽略它
7. **记录异常**：在适当的层级记录异常信息，便于调试
8. **保持异常堆栈**：在重新抛出异常时，保持原始异常的堆栈信息

### 9.2 错误码处理最佳实践

1. **使用枚举定义错误码**：使用枚举提高代码可读性和可维护性
2. **提供错误码到错误信息的映射**：便于将错误码转换为用户友好的错误信息
3. **检查所有错误码**：确保所有错误码都被检查和处理
4. **传递错误上下文**：在返回错误码的同时，提供足够的上下文信息
5. **避免错误码的链式传递**：尽量在适当的层级处理错误，避免错误码在多层函数间传递

### 9.3 可选类型处理最佳实践

1. **使用可选类型表示可能不存在的值**：对于可能不存在的值，使用可选类型而不是特殊值
2. **使用辅助方法处理可选类型**：使用 `?.`、`??` 等辅助方法简化可选类型的处理
3. **在适当的层级处理 null**：在适当的层级检查和处理 null 值，避免 null 传播到深层代码
4. **提供默认值**：对于可选类型，提供合理的默认值

### 9.4 Result 类型处理最佳实践

1. **使用 Result 类型表示可能失败的操作**：对于可能失败的操作，使用 Result 类型同时返回值和错误信息
2. **使用模式匹配处理 Result**：使用模式匹配清晰地处理成功和失败的情况
3. **使用链式调用处理 Result**：使用 `map`、`and_then` 等方法链式处理 Result
4. **提供详细的错误信息**：在 Result 中提供详细的错误信息，便于调试和用户反馈

## 10. 错误处理的性能考虑

### 10.1 异常的性能

- **异常的开销**：异常处理有一定的性能开销，包括异常对象的创建、堆栈展开等
- **异常的使用场景**：异常适合用于真正异常的情况，不适合用于常规的控制流
- **异常的优化**：
  - 只在真正异常的情况下使用异常
  - 避免在性能敏感的热路径中使用异常
  - 使用具体的异常类型，减少异常处理的开销

### 10.2 错误码的性能

- **错误码的性能**：错误码的性能开销很小，适合用于性能敏感的场景
- **错误码的使用场景**：错误码适合用于轻微错误，需要继续执行的情况
- **错误码的优化**：
  - 使用整数或枚举作为错误码，避免使用字符串
  - 合理设计错误码的层次结构，便于处理

### 10.3 可选类型的性能

- **可选类型的性能**：可选类型的性能开销很小，主要是额外的 null 检查
- **可选类型的使用场景**：可选类型适合用于可能不存在的值
- **可选类型的优化**：
  - 合理使用辅助方法，减少重复的 null 检查
  - 在适当的层级处理 null，避免 null 传播到深层代码

### 10.4 Result 类型的性能

- **Result 类型的性能**：Result 类型的性能开销很小，主要是额外的对象创建
- **Result 类型的使用场景**：Result 类型适合用于可能失败的操作，需要同时返回值和错误信息的情况
- **Result 类型的优化**：
  - 合理使用链式调用，减少重复的错误检查
  - 在适当的层级处理错误，避免错误在多层函数间传递

## 11. 错误处理的调试

### 11.1 异常调试

- **异常堆栈**：异常包含完整的堆栈信息，便于定位错误位置
- **异常消息**：异常消息应该包含足够的上下文信息，便于理解错误原因
- **异常日志**：在适当的层级记录异常信息，便于调试
- **调试工具**：使用调试工具查看异常的堆栈和上下文信息

### 11.2 错误码调试

- **错误码映射**：提供错误码到错误信息的映射，便于理解错误原因
- **错误日志**：记录错误码和相关的上下文信息，便于调试
- **错误码追踪**：追踪错误码的传递路径，便于定位错误源头

### 11.3 可选类型调试

- **null 检查**：确保所有可选类型都被适当检查，避免 null 引用异常
- **日志**：在适当的层级记录可选类型的状态，便于调试
- **调试工具**：使用调试工具查看可选类型的状态

### 11.4 Result 类型调试

- **错误信息**：在 Result 中提供详细的错误信息，便于理解错误原因
- **日志**：在适当的层级记录 Result 的状态，便于调试
- **调试工具**：使用调试工具查看 Result 的状态

## 12. 总结

C^ 语言提供了多种错误处理机制，包括异常、错误码、可选类型和 Result 类型等。不同的错误处理机制适用于不同的场景，开发者应该根据具体情况选择合适的错误处理策略。

### 12.1 错误处理机制的选择

- **异常**：适用于严重错误，需要立即中断执行的情况
- **错误码**：适用于轻微错误，需要继续执行的情况
- **可选类型**：适用于可能不存在的值
- **Result 类型**：适用于可能失败的操作，需要同时返回值和错误信息的情况

### 12.2 错误处理的最佳实践

- **一致性**：在项目中保持一致的错误处理风格
- **清晰性**：错误处理代码应该清晰、简洁，易于理解和维护
- **完整性**：处理所有可能的错误情况，避免未处理的异常
- **用户友好**：错误信息应该清晰、准确，便于用户理解和解决问题
- **调试友好**：错误信息应该包含足够的上下文，便于调试

### 12.3 错误处理的未来发展

C^ 语言的错误处理机制将不断发展和完善，未来可能会引入更多的错误处理特性，如：

- **更强大的模式匹配**：用于更清晰地处理错误情况
- **更丰富的错误类型**：提供更具体的错误类型
- **更智能的错误处理**：通过编译器和工具提供更智能的错误处理建议
- **更集成的错误处理**：与其他语言特性更好地集成

通过合理使用 C^ 语言的错误处理机制，可以编写更健壮、更可维护的代码，提高程序的质量和用户体验。