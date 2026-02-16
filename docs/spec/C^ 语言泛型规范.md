# C^ 语言泛型规范

## 1. 概述

C^ 的泛型系统旨在结合 C++ Templates 的**高性能**（编译期展开、零运行时开销）与 Rust/Swift Generics 的**安全性**（强类型约束）。泛型是 C^ 语言的核心特性之一，用于创建类型安全的容器、算法和数据结构。

### 1.1 泛型核心特性

- **编译期展开**：对标 C++ Template，为每个类型生成特定机器码，零运行时开销
- **强类型约束**：使用 Concept（概念）作为泛型约束的核心机制，完全对标 C++20 Concepts
- **结构化类型系统**：隐式实现，无需显式声明实现了某个 Concept
- **Monomorphization**：所有泛型均在编译期实例化，无运行时开销

### 1.2 泛型的优势

- **类型安全**：泛型代码在编译时进行类型检查，避免运行时类型错误
- **代码复用**：泛型代码可以适用于多种类型，减少代码重复
- **性能**：泛型代码在编译时实例化，没有运行时开销
- **可读性**：泛型代码更加清晰、简洁，表达意图更加明确

## 2. 泛型基础

### 2.1 泛型类与结构体

使用尖括号 `<T>` 声明类型参数。类型参数可以在类/结构体内部作为普通类型使用。

#### 语法

```
generic_class_declaration ::= 'class' identifier '<' type_parameter_list '>'
                          [generic_constraint_clause]? class_body
generic_struct_declaration ::= 'struct' identifier '<' type_parameter_list '>'
                           [generic_constraint_clause]? struct_body
type_parameter_list ::= type_parameter (',' type_parameter)*
type_parameter ::= identifier
generic_constraint_clause ::= 'where' constraint_expression
```

#### 示例

```cpp
public class Box<T> {
    private T value_;
    
    public func init(T value) {
        self.value_ = value;
    }
    
    public func get() -> T {
        return self.value_;
    }
}

public struct Point<T> {
    T x;
    T y;
    
    public func new(T x, T y) {
        self.x = x;
        self.y = y;
    }
}

// 使用
Box<int> intBox = new Box<int>(10);
Box<string> strBox = new Box<string>("Hello");

Point<float> p1 = new Point<float>(1.0, 2.0);
Point<int> p2 = new Point<int>(10, 20);
```

### 2.2 泛型函数

泛型参数位于函数名之后，参数列表之前。

#### 语法

```
generic_function_declaration ::= 'func' identifier '<' type_parameter_list '>'
                               '(' parameter_list? ')' ['->' return_type]
                               [generic_constraint_clause]? function_body
```

#### 示例

```cpp
public func swap<T>(T^ a, T^ b) {
    T temp = a^;
    a^ = b^;
    b^ = temp;
}

public func max<T>(T a, T b) -> T where Comparable<T> {
    return a > b ? a : b;
}

// 调用
int x = 5, y = 10;
swap<int>(^x, ^y);
```

## 3. 概念与约束 (Concepts & Constraints)

概念（Concepts）是 C^ 泛型系统的核心，用于对泛型参数施加约束。

### 3.1 定义概念

使用 `concept` 关键字定义概念。概念本质上是一组编译期断言。

#### 语法

```
concept_definition ::= 'concept' identifier '<' type_parameter_list '>' '=' requires_clause ';'
requires_clause ::= 'requires' '(' parameter_list? ')' '{' requirement* '}'
```

#### 示例

```cpp
// 定义一个名为 Addable 的概念
public concept Addable<T> = requires (T a, T b) {
    // 必须支持加法，且结果类型可以转换为 T
    { a + b } -> Convertible<T>;
};
```

### 3.2 需求类型 (Requirement Types)

#### 3.2.1 简单需求 (Simple Requirement)

验证表达式是否合法。

#### 语法

```
simple_requirement ::= expression ';'
```

#### 示例

```cpp
public concept Incrementable<T> = requires (T x) {
    x++;         // 必须支持自增
    x--;         // 必须支持自减
};

public concept Stringifiable<T> = requires (T x) {
    x.to_string(); // 必须有 to_string 方法
};

public concept Container<T> = requires (T x) {
    x.length;     // 必须有 length 属性
    x.empty();    // 必须有 empty 方法
};
```

#### 3.2.2 类型需求 (Type Requirement)

验证类型是否存在（通常用于关联类型）。C^ 移除了 `typename` 关键字，直接使用 `T.Type` 语法。

#### 语法

```
type_requirement ::= nested_name ';'
nested_name ::= identifier ('.' identifier)*
```

#### 示例

```cpp
public concept Iterable<T> = requires (T x) {
    // T 必须有一个名为 Item 的内嵌类型/关联类型
    T.Item;
    
    // T 必须有一个名为 Iterator 的关联类型
    T.Iterator;
};

public concept MapLike<T> = requires (T x) {
    T.Key;
    T.Value;
    
    // 必须有 get 方法，返回 Value
    { x.get(T.Key) } -> T.Value;
};
```

#### 3.2.3 复合需求 (Compound Requirement)

验证表达式合法，**并且**其结果类型满足特定约束。

#### 语法

```
compound_requirement ::= '{' expression '}' '->' type_constraint ';'
type_constraint ::= type_name | concept_name
```

#### 示例

```cpp
public concept Comparable<T> = requires (T a, T b) {
    // a == b 必须合法，且返回值必须能转换为 bool
    { a == b } -> bool;
    { a != b } -> bool;
    
    // a < b 必须合法，且返回值必须是 bool
    { a < b } -> bool;
    { a > b } -> bool;
    { a <= b } -> bool;
    { a >= b } -> bool;
};

public concept Comparator<T> = requires (T a, T b) {
    // a.compare(b) 必须合法，且返回值必须是 int
    { a.compare(b) } -> int;
};

public concept Stream<T> = requires (T x) {
    // x.read() 必须合法，且返回值必须是 byte[]
    { x.read() } -> byte[];
    
    // x.write(data) 必须合法，且返回值必须是 int
    { x.write(byte[]) } -> int;
};
```

#### 3.2.4 嵌套需求 (Nested Requirement)

在 `requires` 块中引用其他概念。必须以 `requires` 开头。

#### 语法

```
nested_requirement ::= 'requires' concept_name '<' type_list? '>' ';'
```

#### 示例

```cpp
public concept Swappable<T> = requires (T a, T b) {
    // 必须有 swap 函数
    swap(a, b);
};

public concept Comparable<T> = requires (T a, T b) {
    { a < b } -> bool;
};

public concept Sortable<T> = requires (T x) {
    // T 必须先满足 Swappable 概念
    requires Swappable<T>;
    
    // T 必须满足 Comparable 概念
    requires Comparable<T>;
    
    // 必须有 sort 方法
    x.sort();
};

public concept Container<T> = requires (T x) {
    T.Item;
    requires Comparable<T.Item>;
    
    { x.length } -> int;
    { x.get(int) } -> T.Item;
};
```

### 3.3 使用约束 (Applying Constraints)

约束可以通过两种主要方式应用：`where` 子句。

#### 3.3.1 `where` 子句 (推荐)

`where` 子句位于函数签名或类声明的末尾，提供最清晰的约束列表。支持 `&&` (与) 和 `||` (或) 逻辑组合。

#### 语法

```
generic_constraint_clause ::= 'where' constraint_expression
constraint_expression ::= 
    concept_name '<' type_list '>'
  | constraint_expression '&&' constraint_expression
  | constraint_expression '||' constraint_expression
  | '(' constraint_expression ')'
```

#### 示例

```cpp
// 单一约束
public func max<T>(T a, T b) -> T 
    where Comparable<T> 
{
    return a > b ? a : b;
}

// 组合约束
public func serialize<T>(T obj) 
    where Serializable<T> && Debuggable<T>
{
    // ...
}

### 3.4 约束语法区分：继承 vs 概念

为了明确区分**面向对象继承**（运行时多态/子类型）和**泛型概念**（编译期鸭子类型），C^ 采用了不同的约束语法：

1.  **继承约束 (Inheritance Constraint)**: 使用 `:` 语法。
    *   **语法**: `where T : BaseClass` 或 `where T : IInterface`
    *   **语义**: 要求 `T` 必须是 `BaseClass` 的子类或实现了 `IInterface`。这通常意味着 `T` 必须具有特定的内存布局或虚表指针。
    
2.  **概念约束 (Concept Constraint)**: 使用 `Concept<T>` 语法。
    *   **语法**: `where ConceptName<T>`
    *   **语义**: 要求 `T` 满足 `ConceptName` 定义的编译期契约（鸭子类型）。不要求 `T` 继承自任何特定基类，只要具备所需的成员函数/操作符即可。**这是实现零开销抽象的首选方式**。

```cpp
// 继承约束：T 必须是 Widget 的子类 (强耦合)
func process_widget<T>(T w) where T : Widget { ... }

// 概念约束：T 只需要满足 Drawable 概念 (松耦合，零开销)
func draw_object<T>(T obj) where Drawable<T> { ... }
```

// 涉及多个泛型参数的约束
public func zip<A, B>(seq1: A, seq2: B) -> Pair<A::Item, B::Item>
    where Iterable<A> && Iterable<B> && SameType<A::Item, B::Item>
{
    // 实现 zip 逻辑
}

// 复杂约束组合
public func process<T>(T data) 
    where (Numeric<T> || String<T>) && Copyable<T> && Movable<T>
{
    // 处理逻辑
}
```

### 3.4 关联类型约束

对于像迭代器这样的 Traits，经常需要约束其关联类型。

#### 示例

```cpp
// 定义迭代器概念
public concept Iterator<T> = requires (T x) {
    T.Item;
    { x.next() } -> T.Item?;
};

// 约束：C 必须是 Iterator，且其元素类型必须是 string
public func print_strings<C>(C container)
    where Iterator<C> && SameType<C.Item, string>
{
    foreach (s in container) {
        print(s);
    }
}

// 关联类型在约束中的使用
public concept Container<T> = requires (T x) {
    T.Item;
    T.Iterator;
    requires Iterator<T.Iterator>;
    { x.begin() } -> T.Iterator;
    { x.end() } -> T.Iterator;
};
```

## 4. 标准库常用概念 (Standard Concepts)

为了避免开发者重复造轮子，C^ 标准库 (`std.concepts`) 预定义了一组基础概念。

### 4.1 类型关系概念

| 概念名              | 定义简述                    | 用途         |
| ------------------- | --------------------------- | ------------ |
| `SameType<T, U>`    | `T` 与 `U` 是完全相同的类型 | 类型严格匹配 |
| `Derived<T, U>`     | `T` 继承自 `U`              | OOP 继承约束 |
| `Convertible<T, U>` | `T` 可以隐式转换为 `U`      | 类型转换     |
| `Common<T, U>`      | `T` 和 `U` 拥有公共类型     | 混合运算     |

#### 示例

```cpp
public func copy<T, U>(T source, U target) -> U 
    where Convertible<T, U>
{
    return source;
}

public func is_same_type<T, U>(T a, U b) -> bool
    where SameType<T, U>
{
    return true;
}
```

### 4.2 运算符概念

| 概念名                  | 定义简述                   | 用途           |
| ----------------------- | -------------------------- | -------------- |
| `Boolean<T>`            | `T` 可以像 `bool` 一样使用 | 条件判断       |
| `EqualityComparable<T>` | 支持 `==` 和 `!=`          | 相等性比较     |
| `Comparable<T>`         | 支持 `<, >, <=, >=`        | 排序与大小比较 |
| `Addable<T>`            | 支持 `+` 且结果类型一致    | 加法运算       |
| `Subtractable<T>`       | 支持 `-` 且结果类型一致    | 减法运算       |
| `Multipliable<T>`       | 支持 `*` 且结果类型一致    | 乘法运算       |
| `Divisible<T>`          | 支持 `/` 且结果类型一致    | 除法运算       |

#### 示例

```cpp
public func sum<T>(T[] items) -> T 
    where Addable<T> && Copyable<T>
{
    T result = items[0];
    for (int i = 1; i < items.length; i++) {
        result = result + items[i];
    }
    return result;
}

public func binary_search<T>(T[] arr, T target) -> int
    where Comparable<T>
{
    // 实现二分查找
}
```

### 4.3 语义概念

| 概念名                    | 定义简述               | 用途       |
| ------------------------- | ---------------------- | ---------- |
| `Copyable<T>`             | `T` 支持拷贝构造和赋值 | 值语义约束 |
| `Movable<T>`              | `T` 支持移动语义       | 资源管理   |
| `Destructible<T>`         | `T` 有析构函数         | 资源清理   |
| `DefaultConstructible<T>` | `T` 有默认构造函数     | 默认初始化 |

#### 示例

```cpp
public func clone<T>(T obj) -> T
    where Copyable<T>
{
    return T(obj); // 拷贝构造
}
```

## 5. 万能引用与完美转发 (Universal Reference & Perfect Forwarding)

C^ 提供了**万能引用**（Universal Reference）和**完美转发**（Perfect Forwarding）机制，以支持高效的泛型编程，避免不必要的拷贝。

### 5.1 万能引用符号 `T&~`

C^ 引入了显式的万能引用运算符 `&~`（Left-Right Combined Operator），仅在泛型函数参数中使用，明确表示该参数可以接受左值或右值。

```cpp
// 语法：T &~ param
func<T> wrapper(T &~ arg) {
    // ...
}
```

#### 引用折叠与类型推导规则

当使用 `T &~` 时，编译器根据传入实参的值类别进行类型推导：

| 实参类别         | 推导出的 `T` | 参数 `arg` 的实际类型                  |
| :--------------- | :----------- | :------------------------------------- |
| **左值** (int x) | `int&`       | `int&` (引用折叠: `int& &~` -> `int&`) |
| **右值** (10)    | `int`        | `int~` (引用折叠: `int &~` -> `int~`)  |

> **设计哲学**：`&~` 运算符直观地展示了其"既可左 (`&`) 也可右 (`~`)"的特性，消除了 C++ 中 `T&&` 既是右值引用又是万能引用的歧义。
>
> **对比分析**：
> | 特性 | C++ 语法 | C^ 语法 | 歧义性分析 |
> | :--- | :--- | :--- | :--- |
> | **右值引用** | `Widget&&` | `Widget~` | C++ 中 `&&` 含义取决于上下文（是否发生类型推导）。 |
> | **万能引用** | `T&&` (需推导) | `T&~` | C^ 中 `&~` 专用于泛型推导，`~` 专用于右值，语义完全分离，降低了心智负担。 |

### 5.2 完美转发 `forward<T>`

`forward<T>` 利用上述推导出的 `T` 类型信息，还原参数的原始值类别。

- 如果 `T` 是引用类型 (`U&`)，`forward` 返回左值引用。
- 如果 `T` 是非引用类型 (`U`)，`forward` 返回右值引用。

#### 示例：转发包装器

```cpp
func process(int& x) { print("Lvalue"); }
func process(int~ x) { print("Rvalue"); }

// 万能引用包装器
func<T> logAndProcess(T &~ arg) {
    print("Log: processing...");
    // 完美转发：arg 如果是左值则传左值，是右值则传右值
    process(forward<T>(arg));
}

int a = 10;
logAndProcess(a);       // T=int&, arg=int&. forward<int&> -> int&. 输出 "Lvalue"
logAndProcess(20);      // T=int, arg=int~. forward<int> -> int~. 输出 "Rvalue"
```

### 5.3 完美转发构造函数

```cpp
class Widget {
    string name;
    
    // 构造函数接受万能引用
    func<U> init(U &~ n) {
        // 将 n 转发给 string 的构造/赋值
        self.name = forward<U>(n);
    }
}
```
