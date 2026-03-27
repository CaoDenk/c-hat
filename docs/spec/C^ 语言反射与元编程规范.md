# C^ 语言反射与元编程规范

## 1. 概述

C^ 的反射机制是**纯静态（Compile-time）**的，零运行时开销，零标准库依赖。`@` 操作符和 `comptime for` 是纯编译器内置特性，所有元数据属性类型均为内置类型（`literalview`、`bool`、`int`、`long`），不引用任何 `std.*` 模块。

运行时反射是独立的 opt-in 机制（见第 4 节），与静态反射完全分离。

## 2. 核心概念

- **编译期反射**：`@` 操作符 + `comptime for`，编译器内置，零运行时开销
- **元编程**：`comptime` 关键字标记编译期执行的代码或函数
- **运行时反射**：`[Reflectable]` 标注 + `obj.rtti`，opt-in，默认不生成

## 3. 编译期反射

### 3.1 设计原则

1. **零标准库依赖** — `@` 与 `comptime for` 纯编译器内置
2. **元数据类型全用内置类型** — `literalview`、`bool`、`int`、`long`
3. **类型是一等公民** — `field.type` 返回真正的编译期类型值，不是字符串
4. **编译期/运行时严格分离**

### 3.2 `typeof` — 获取表达式的静态类型

`typeof` 是关键字，返回编译期类型值，可用于声明、泛型参数、类型比较：

```c^
int a = 10;
typeof(a) b = 20;           // 等价于 int b = 20
List<typeof(a)> list;       // 等价于 List<int>

// 类型比较（编译期）
comptime if (field.type == typeof(int)) { ... }
```

### 3.3 `@` 操作符 — 获取类型的编译期元对象

`@` 是前缀操作符，作用于**类型**，返回编译器生成的匿名元对象：

```c^
@User              // 获取 User 类型的元对象
@typeof(expr)      // 先取静态类型，再取元对象
@int               // 内置类型同样支持
```

> ⚠️ **`@` 永远作用于类型，不直接作用于值。**
> - `@User` — 合法，`User` 是类型名
> - `@typeof(a)` — 合法，`typeof(a)` 在编译期返回 `a` 的静态类型
> - `@a` — **编译错误**，`a` 是值

`@` 操作符**不可重载**，保证反射入口的统一性和确定性。

### 3.4 元对象的内置属性

所有属性均为编译期常量，类型全部是内置类型：

```c^
@T.name            // literalview  — 类型名，如 "User"
@T.is_struct       // bool
@T.is_class        // bool
@T.is_enum         // bool
@T.is_primitive    // bool         — int/bool/byte/char/long/float/double
@T.size            // long         — sizeof(T)
@T.align           // long         — alignof(T)
@T.field_count     // int
@T.fields          // 编译期异构元组，只能 comptime for 遍历
@T.method_count    // int
@T.methods         // 编译期异构元组，只能 comptime for 遍历
```

**字段元对象：**

```c^
field.name         // literalview  — 字段名
field.type         // 编译期类型值  — 可与 typeof() 比较
field.offset       // long         — 字节偏移
field.is_public    // bool
field.is_mutable   // bool         — var 为 true，let 为 false
field.has_attr(A)  // bool         — 是否有 Attribute A
field.attr(A)      // A 的编译期实例
```

**方法元对象：**

```c^
method.name        // literalview
method.is_public   // bool
method.is_static   // bool
method.param_count // int
```

### 3.5 `comptime for` — 编译期异构展开

`comptime for` 遍历 `@T.fields` / `@T.methods`，编译器将循环**完全展开**为独立语句，不产生任何运行时循环：

```c^
func print_fields<T>(T! obj) {
    comptime for (var field : @T.fields) {
        print(field.name);
        print(": ");
        print(obj.[field]);   // 通过字段元对象访问成员
        print("\n");
    }
}
```

### 3.6 字段访问语法 `obj.[field]`

通过字段元对象访问对象成员，与硬编码 `obj.name` 完全等价：

```c^
obj.[field]           // 读取字段值
obj.[field] = value   // 写入字段（field.is_mutable == true 时合法）
```

### 3.7 `comptime if` — 编译期条件分支

```c^
comptime for (var field : @T.fields) {
    comptime if (field.type == typeof(int) || field.type == typeof(long)) {
        print_int(obj.[field]);
    } else comptime if (field.type == typeof(bool)) {
        print_bool(obj.[field]);
    } else comptime if (field.type == typeof(literalview)) {
        print_str(obj.[field]);
    } else {
        print_fields(obj.[field]);   // 递归处理嵌套类型
    }
}
```

## 4. 运行时反射 (Runtime Reflection)

### 4.1 设计原则：严格 Opt-in

运行时反射与静态反射**完全独立**，不共用任何机制。C^ 默认**不生成** RTTI，保证零开销和最小二进制体积。

### 4.2 启用运行时反射

```c^
[Reflectable]
class Animal { ... }

[Reflectable]
class Dog : Animal { ... }
```

编译器为 `[Reflectable]` 类型注入最小 `rtti` 字段，只包含动态类型名。

### 4.3 使用

```c^
Animal^ a = new Dog;
a.rtti.name    // literalview，运行时可用，值为 "Dog"（动态类型名）
```

对于未标注 `[Reflectable]` 的类型，`rtti` 字段不存在，访问即**编译错误**。

### 4.4 原则

- 不生成全局类型表，只为 `[Reflectable]` 类型注入最小字段
- 运行时反射不依赖静态反射的 `@` 操作符
- 默认关闭，不产生任何二进制开销

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