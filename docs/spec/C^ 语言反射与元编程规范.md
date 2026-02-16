# C^ 语言反射与元编程规范

## 1. 概述

C^ 语言提供了强大的反射和元编程能力，允许程序在编译期和运行时检查和操作类型信息。反射和元编程是 C^ 语言的高级特性，用于实现序列化、ORM、依赖注入等复杂功能。本文档详细描述 C^ 语言中反射和元编程的语法、API 和应用场景。

## 2. 反射基础

### 2.1 反射概念

- **反射**：程序在运行时或编译期检查和操作自身结构的能力
- **编译期反射**：在编译期获取和操作类型信息，零运行时开销
- **运行时反射**：在运行时获取和操作类型信息，有一定运行时开销
- **元编程**：编写能够生成或修改其他代码的代码

### 2.2 反射的优势

- **代码生成**：通过反射自动生成重复性代码
- **动态类型操作**：在运行时处理未知类型
- **序列化和反序列化**：自动将对象转换为其他格式（如 JSON、XML）
- **依赖注入**：自动解析和注入依赖
- **ORM**：对象关系映射，自动处理数据库操作
- **测试工具**：自动生成测试代码和测试数据

## 3. 编译期反射

### 3.1 设计哲学

C^ 的反射机制是**纯静态（Compile-time）**的。与 Java/C# 的运行时反射不同，C^ 的反射在编译期完成所有元信息的解析和代码生成。

### 3.2 核心语法：`@` 操作符与 `typeof` 关键字

我们将 **类型获取** 与 **元数据获取** 分离，实现更优雅的正交设计。

#### 3.2.1 类型操作符：`typeof(expr)`

`typeof` 是一个 **关键字**，用于在编译期获取表达式的类型。它返回的是一个 **类型 (Type)**，可以用于变量声明、泛型参数等。

```cpp
int a = 10;
typeof(a) b = 20; // 等价于 int b = 20;
List<typeof(a)> list; // 等价于 List<int>
```

#### 3.2.2 反射操作符：`@`

`@` 是一个 **前缀运算符**，用于获取类型或实体的 **元数据 (Metadata)**。它返回一个编译期常量对象（如 `std::meta::Type`）。

*   语法：`@Type`
*   组合：`@typeof(expr)` —— 先获取类型，再获取该类型的元数据。

```cpp
struct User { int id; }
int a = 10;

// 1. 获取命名类型的元数据
comptime TypeInfo info1 = @User;

// 2. 获取表达式类型的元数据 (组合使用)
comptime TypeInfo info2 = @typeof(a); 

// 3. 错误示例
// comptime TypeInfo info3 = @a; // 错误！@ 操作符作用于类型，而非值
```

这种设计的优势在于：
1.  **正交性**：`typeof` 负责类型系统，`@` 负责反射系统。
2.  **清晰性**：`@typeof(a)` 明确表达了"获取 a 的类型的元数据"这一语义。
3.  **扩展性**：`@` 未来可以扩展用于获取函数、模块的元数据（如 `@func_name`）。

#### 3.2.3 `std.meta.Type` 接口

`@T` 返回的对象提供了一系列编译期属性：

```cpp
// 编译期获取类型名
comptime string type_name = @typeof(User).name; // "User"

// 判断是否为结构体
comptime bool is_struct = @typeof(User).is_struct;
```

#### 3.2.4 `@` 操作符不可重载

`@` 是编译器内置的反射操作符，**不允许用户重载**。这保证了反射行为的确定性和零开销。用户如果需要自定义类型的反射行为，应通过 **Attributes (属性)** 或 **Traits/Concepts** 来实现。

### 3.3 编译期遍历：`comptime for`

摒弃奇怪的 `@foreach`，直接复用 `for` 循环，加上 `comptime` 标记，表示这是**编译期展开**。

```cpp
func print_fields<T>(T! obj) {
    // 编译期遍历 T 的所有字段
    // 编译器会将此循环展开为一系列顺序执行的语句
    comptime for (var field : @T.fields) {
        // 生成代码：print("id: " + obj.id);
        // 生成代码：print("name: " + obj.name);
        print(field.name + ": " + obj.@[field]); 
    }
}
```

### 3.4 成员访问：`@[meta]`

`obj.@[meta]` 用于通过元数据访问对象的成员。这是一种**编译期成员访问**语法。

### 3.5 类型元数据概念 (Meta Concept)

`@T` 返回的元类型满足以下 Concept 约束（而非固定的 Interface）：

```cpp
concept TypeInfo {
    // 类型基本信息 (comptime 常量)
    comptime bool is_void;
    comptime bool is_struct;
    // ...
    
    // 属性
    comptime string name;
    
    // 成员集合 (编译期可遍历序列)
    comptime auto fields; // Returns Sequence<FieldInfo>
    comptime auto methods; // Returns Sequence<MethodInfo>
}
```

## 4. 运行时反射 (Runtime Reflection)

### 4.1 设计原则：Opt-in RTTI

C^ 默认**不生成**运行时类型信息 (RTTI)，以保证零开销和最小二进制体积。
运行时反射是**基于静态反射构建的**。

### 4.2 启用运行时反射

用户必须显式标记需要运行时反射的类型：

```cpp
[Reflectable]
class User { ... }

// 或者在编译选项中开启全局 RTTI（不推荐）
```

对于未标记的类型，`@typeof(val)` 在运行时可能会报错或返回有限信息。

### 4.3 `@typeof` 运算符

`@typeof` 运算符在运行时获取类型的元数据（如果已启用）：

```cpp
// 运行时获取类型元数据
func get_type_info<T>(T value) -> TypeInfo {
    return @typeof(value);
}
```

## 5. 编译期执行 (Comptime Execution)

### 5.1 `comptime` 关键字

`comptime` 关键字用于标记在编译期执行的代码：

```cpp
// 编译期计算
comptime {
    const int FACTORIAL_10 = {
        int result = 1;
        for (int i = 1; i <= 10; i++) {
            result *= i;
        }
        result;
    };
}

// 使用编译期计算结果
func get_factorial_10() -> int {
    return FACTORIAL_10; // 编译期常量，值为 3628800
}

// 编译期函数
comptime func factorial(int n) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 使用编译期函数
const int FACT_5 = factorial(5); // 编译期计算，值为 120
```

### 5.2 Comptime 类型操作

使用 `comptime` 进行编译期类型操作：

```cpp
// 编译期类型判断
comptime func is_integral_type(TypeInfo type) -> bool {
    return type.is_int || type.is_long || type.is_short || 
           type.is_byte || type.is_uint || type.is_ulong || 
           type.is_ushort || type.is_ubyte;
}

// 编译期类型映射
comptime func map_integral_type(TypeInfo type) -> TypeInfo {
    if (type.is_int) {
        return typeof(long);
    } else if (type.is_long) {
        return typeof(int);
    } else if (type.is_short) {
        return typeof(int);
    } else {
        return type;
    }
}

// 使用
comptime {
    static_assert(is_integral_type(typeof(int)), "int should be integral");
    static_assert(!is_integral_type(typeof(double)), "double should not be integral");
    
    using MappedType = map_integral_type(typeof(int));
    static_assert(MappedType == typeof(long));
}
```

### 5.3 Comptime 代码生成

使用 `comptime` 进行编译期代码生成：

```cpp
// 编译期生成函数
comptime func generate_adder(int n) -> func(int) -> int {
    return func(int x) -> int {
        return x + n;
    };
}

// 使用编译期生成的函数
const auto add5 = generate_adder(5);
const auto add10 = generate_adder(10);

int result1 = add5(10); // 15
int result2 = add10(10); // 20

// 编译期生成结构体
comptime func generate_point_struct(int dimensions) -> TypeInfo {
    // 生成 PointN 结构体
    // ...
    return generated_type;
}

// 使用
using Point3D = generate_point_struct(3);
Point3D p = {1, 2, 3};
```

### 5.4 Comptime 条件编译

使用 `comptime` 进行条件编译：

```cpp
// 编译期条件
comptime {
    const bool IS_DEBUG = true;
    const bool IS_64_BIT = sizeof(void*) == 8;
}

// 条件编译
comptime if (IS_DEBUG) {
    func debug_log(string message) -> void {
        println("[DEBUG] " + message);
    }
} else {
    func debug_log(string message) -> void {
        // 空实现
    }
}

// 平台特定代码
comptime if (IS_64_BIT) {
    func get_platform() -> string {
        return "64-bit";
    }
} else {
    func get_platform() -> string {
        return "32-bit";
    }
}
```

## 6. 元编程技术

### 6.1 模板元编程

C^ 支持模板元编程，使用编译期执行和类型操作：

```cpp
// 模板元编程：计算阶乘
comptime func factorial(int n) -> int {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

// 模板元编程：类型列表
comptime func<Ts...> type_list_length() -> int {
    return sizeof...(Ts);
}

// 模板元编程：类型索引
comptime func<Ts..., int N> get_type_at() -> TypeInfo {
    static_assert(N < sizeof...(Ts), "Index out of bounds");
    
    TypeInfo[] types = [typeof(Ts)...];
    return types[N];
}

// 使用
static_assert(factorial(5) == 120);
static_assert(type_list_length<int, double, string>() == 3);
static_assert(get_type_at<int, double, string, 1>() == typeof(double));
```

### 6.2 编译期反射元编程

使用编译期反射进行元编程：

```cpp
// 编译期反射：自动生成属性访问器
comptime func generate_property_accessors(TypeInfo type) -> void {
    for (auto field in type.fields) {
        if (field.is_public) {
            // 生成 getter
            string getter_name = "get_" + field.name;
            // 生成 setter
            string setter_name = "set_" + field.name;
            // 生成代码
            // ...
        }
    }
}

// 编译期反射：自动生成序列化代码
comptime func generate_serializer(TypeInfo type) -> void {
    // 生成序列化代码
    // ...
}

// 编译期反射：自动生成比较操作
comptime func generate_comparison_operators(TypeInfo type) -> void {
    // 生成 ==, !=, <, >, <=, >= 操作符
    // ...
}
```

### 6.3 代码生成

使用元编程技术生成代码：

```cpp
// 代码生成：自动生成枚举字符串转换
#define GENERATE_ENUM_STRINGS(EnumType, ...) \
comptime { \
    const string[] _##EnumType##_strings = {__VA_ARGS__}; \
} \
func EnumType##_to_string(EnumType value) -> string { \
    return _##EnumType##_strings[(int)value]; \
} \
func string_to_##EnumType(string name) -> EnumType? { \
    for (int i = 0; i < _##EnumType##_strings.length; i++) { \
        if (_##EnumType##_strings[i] == name) { \
            return (EnumType)i; \
        } \
    } \
    return null; \
}

// 使用
enum Color {
    Red,
    Green,
    Blue
}

GENERATE_ENUM_STRINGS(Color, "Red", "Green", "Blue");

// 使用生成的函数
Color c = Color::Red;
string color_name = Color_to_string(c); // "Red"
Color? parsed_color = string_to_Color("Green"); // Color::Green
```

## 7. 反射与元编程的应用

### 7.1 序列化

使用反射实现自动序列化：

```cpp
// 自动序列化
func<T> serialize(T obj) -> string {
    TypeInfo type = @typeof(obj);
    string result = "{";
    
    bool first = true;
    for (auto field in type.fields) {
        if (!first) {
            result += ", ";
        }
        result += "\"" + field.name + "\": ";
        
        if (field.type.is_int) {
            result += field.get_value(obj).ToString();
        } else if (field.type.is_string) {
            result += "\"" + field.get_value(obj).ToString() + "\"";
        } else if (field.type.is_bool) {
            result += field.get_value(obj).ToString().ToLower();
        } else {
            // 递归序列化
            result += serialize(field.get_value(obj));
        }
        
        first = false;
    }
    
    result += "}";
    return result;
}

// 使用
class Person {
    public string name;
    public int age;
    public bool active;
}

Person p = {"Alice", 30, true};
string json = serialize(p); // {"name": "Alice", "age": 30, "active": true}
```

### 7.2 依赖注入

使用反射实现依赖注入：

```cpp
// 依赖注入容器
class DependencyContainer {
    private map<TypeInfo, void^> _services;
    
    public func register<T>(T instance) -> void {
        _services[typeof(T)] = (void^)instance;
    }
    
    public func resolve<T>() -> T? {
        TypeInfo type = typeof(T);
        if (_services.contains_key(type)) {
            return (T)_services[type];
        }
        return null;
    }
    
    public func create_instance<T>() -> T? {
        TypeInfo type = typeof(T);
        
        // 查找无参构造函数
        ConstructorInfo? ctor = null;
        for (auto constructor in type.constructors) {
            if (constructor.parameters.length == 0) {
                ctor = constructor;
                break;
            }
        }
        
        if (ctor != null) {
            return (T)ctor.invoke([]);
        }
        
        return null;
    }
}

// 使用
class Logger {
    public func log(string message) -> void {
        println(message);
    }
}

class UserService {
    private Logger^ _logger;
    
    public func init(Logger^ logger) {
        _logger = logger;
    }
    
    public func get_user(int id) -> User {
        _logger.log("Getting user " + id.ToString());
        // 获取用户逻辑
        return User();
    }
}

// 注册和解析
DependencyContainer container;
container.register(new Logger());
UserService^ user_service = container.create_instance<UserService>();
```

### 7.3 ORM

使用反射实现对象关系映射：

```cpp
// 简单 ORM
class EntityFramework {
    public func save<T>(T entity) -> void {
        TypeInfo type = @typeof(entity);
        string table_name = type.name;
        
        string insert_sql = "INSERT INTO " + table_name + " (";
        string values_sql = "VALUES (";
        
        bool first = true;
        for (auto field in type.fields) {
            if (!first) {
                insert_sql += ", ";
                values_sql += ", ";
            }
            insert_sql += field.name;
            values_sql += "?";
            first = false;
        }
        
        insert_sql += ") " + values_sql + ")";
        
        // 执行 SQL
        println("Executing: " + insert_sql);
        // 绑定参数并执行
    }
    
    public func query<T>(string condition) -> T[] {
        TypeInfo type = typeof(T);
        string table_name = type.name;
        
        string select_sql = "SELECT * FROM " + table_name;
        if (!condition.empty()) {
            select_sql += " WHERE " + condition;
        }
        
        // 执行查询
        println("Executing: " + select_sql);
        // 执行查询并映射结果
        return [];
    }
}

// 使用
class User {
    public int id;
    public string name;
    public int age;
}

EntityFramework db;
User user = {1, "Alice", 30};
db.save(user);
User[] users = db.query<User>("age > 25");
```

### 7.4 测试框架

使用反射实现测试框架：

```cpp
// 简单测试框架
class TestRunner {
    private struct TestCase {
        string name;
        func() -> void test_func;
    };
    
    private TestCase[] _test_cases;
    
    public func register_test(string name, func() -> void test_func) -> void {
        _test_cases.push({name, test_func});
    }
    
    public func run_all_tests() -> void {
        int passed = 0;
        int failed = 0;
        
        for (auto test_case in _test_cases) {
            println("Running test: " + test_case.name);
            try {
                test_case.test_func();
                println("Test passed: " + test_case.name);
                passed++;
            } catch (Exception^ e) {
                println("Test failed: " + test_case.name);
                println("Error: " + e.message);
                failed++;
            }
        }
        
        println("Test summary: " + passed + " passed, " + failed + " failed");
    }
};

// 使用反射自动发现测试
comptime func discover_tests(TypeInfo type) -> void {
    for (auto method in type.methods) {
        if (method.name.starts_with("test_")) {
            TestRunner.register_test(method.name, method.create_delegate());
        }
    }
}

// 测试类
class MathTests {
    public func test_add() -> void {
        assert(add(1, 2) == 3);
    }
    
    public func test_subtract() -> void {
        assert(subtract(5, 3) == 2);
    }
}

// 自动发现测试
comptime {
    discover_tests(typeof(MathTests));
}

// 运行测试
TestRunner.run_all_tests();
```

## 8. 反射与元编程的性能

### 8.1 编译期反射性能

- **编译时间**：编译期反射会增加编译时间，特别是复杂的元编程
- **运行时性能**：编译期反射生成的代码与手写代码性能相同，零运行时开销
- **二进制大小**：编译期反射可能会增加二进制大小，因为生成了额外的代码

### 8.2 运行时反射性能

- **运行时开销**：运行时反射有一定的运行时开销，因为需要在运行时查找类型信息
- **缓存**：使用缓存可以减少反射的运行时开销
- **使用场景**：运行时反射适合在启动时或配置时使用，不适合在性能敏感的热路径中使用

### 8.3 性能优化

反射和元编程的性能优化建议：

1. **优先使用编译期反射**：尽可能使用编译期反射，避免运行时开销
2. **缓存反射结果**：对于运行时反射，缓存类型信息和方法描述符
3. **减少反射使用**：只在必要时使用反射，避免过度使用
4. **使用代码生成**：对于重复性代码，使用编译期代码生成
5. **优化编译时间**：对于复杂的元编程，考虑分解为多个较小的编译单元

## 9. 反射与元编程最佳实践

### 9.1 设计原则

1. **类型安全**：使用反射时确保类型安全，避免运行时类型错误
2. **代码清晰**：反射和元编程代码应该清晰、可维护，避免过度复杂化
3. **性能考虑**：在性能敏感的场景中谨慎使用反射
4. **错误处理**：为反射操作添加适当的错误处理
5. **文档**：为使用反射和元编程的代码添加详细的文档

### 9.2 使用建议

1. **序列化和反序列化**：使用反射自动生成序列化代码
2. **依赖注入**：使用反射实现依赖注入容器
3. **ORM**：使用反射实现对象关系映射
4. **测试框架**：使用反射自动发现和运行测试
5. **代码生成**：使用编译期元编程生成重复性代码
6. **配置系统**：使用反射处理配置信息
7. **插件系统**：使用反射加载和管理插件

### 9.3 常见陷阱

1. **性能问题**：在热路径中过度使用运行时反射
2. **类型安全**：反射操作可能导致运行时类型错误
3. **代码可读性**：过度使用元编程可能导致代码难以理解
4. **编译时间**：复杂的元编程可能显著增加编译时间
5. **二进制大小**：过度使用代码生成可能增加二进制大小
6. **版本兼容性**：反射代码可能对类型结构的变化敏感

## 10. 总结

C^ 语言提供了强大的反射和元编程能力，包括：

- **编译期反射**：使用 `typeof` 运算符在编译期获取类型信息
- **运行时反射**：使用 `@typeof` 运算符在运行时获取类型信息
- **Comptime 编译期执行**：使用 `comptime` 关键字在编译期执行代码
- **元编程技术**：包括模板元编程、编译期反射元编程和代码生成

反射和元编程是 C^ 语言的高级特性，用于实现序列化、依赖注入、ORM、测试框架等复杂功能。通过合理使用反射和元编程，可以编写更灵活、更强大的代码，减少重复性工作，提高代码质量和可维护性。

然而，反射和元编程也有其局限性，包括编译时间增加、运行时开销（对于运行时反射）和代码复杂性增加。在使用反射和元编程时，应该权衡其优缺点，在适当的场景中使用，并遵循最佳实践以确保代码的性能、可读性和可维护性。