# C^ 语言类与面向对象规范

## 1. 概述

C^ 语言支持面向对象编程范式，提供了类、继承、多态等核心特性。本文档详细描述 C^ 语言中类的声明、继承、多态、封装、抽象等面向对象编程的各个方面。

## 2. 类的声明与定义

### 2.1 基本类声明

类是面向对象编程的基本单位，用于封装数据和行为：

```cpp
// 基本类声明
class Person {
    // 成员变量
    string name;
    int age;

    // 成员函数
    func greet() -> string {
        return "Hello, my name is " + name;
    }

    func set_age(int new_age) -> void {
        age = new_age;
    }

    func get_age() -> int {
        return age;
    }
}
```

### 2.2 访问修饰符

C^ 提供了四种访问修饰符，采用**成员修饰符风格**（类似 Java/C#），而非 C++ 的块状风格：

| 修饰符      | 可见性                              |
| ----------- | ----------------------------------- |
| `public`    | 公开的，任何地方都可以访问          |
| `private`   | 私有的，只能在类内部访问            |
| `protected` | 受保护的，只能在类内部和子类中访问  |
| `internal`  | 模块可见，当前模块/包内可见（默认） |

```cpp
// 带访问修饰符的类
class Person {
    // 私有成员
    private string name_;
    private int age_;

    // 公开属性
    public get name -> string {
        return name_;
    }
    public set name(string v) {
        name_ = v;
    }

    // 公开成员函数
    public func greet() -> string {
        return "Hello, my name is " + name_;
    }

    // 受保护成员
    protected func set_age(int new_age) -> void {
        age_ = new_age;
    }

    // 内部成员（默认 internal）
    func calculate_tax() -> int {
        return age_ * 10;
    }
}
```

**默认访问级别**：如果没有指定访问修饰符，成员默认为 `internal`（当前模块/包内可见）。

> **关于 friend**：C^ 移除了 C++ 的 `friend` 关键字。我们认为 `friend` 破坏了封装性，且容易导致紧耦合。
> 推荐使用 `internal` 修饰符来实现模块内的友元访问，或者通过公开受控的接口。

## 3. 构造函数与析构函数

### 3.1 构造函数

构造函数用于初始化类的实例，函数名与类名相同：

```cpp
class Person {
    string name;
    int age;

    // 构造函数
    Person(string new_name, int new_age) {
        name = new_name;
        age = new_age;
    }

    // 无参构造函数
    Person() {
        name = "Unknown";
        age = 0;
    }

    // 带默认参数的构造函数
    Person(string new_name) {
        name = new_name;
        age = 0;
    }
}

// 创建实例
Person^ p1 = new Person("Alice", 30); // 调用第一个构造函数
Person^ p2 = new Person(); // 调用第二个构造函数
Person^ p3 = new Person("Bob"); // 调用第三个构造函数
```

### 3.2 析构函数

析构函数用于清理类的实例，使用 `~ClassName() { ... }` 语法：

```cpp
class Resource {
    void^ handle;

    // 构造函数
    public Resource() {
        handle = allocate_resource();
    }

    // 析构函数
    public ~Resource() {
        if (handle != null) {
            free_resource(handle);
            handle = null;
        }
    }
}

// 使用资源
Resource^ r = new Resource();
// 使用 r
// 离开作用域时，r 的析构函数会被调用，自动清理资源
```

#### 接口声明

在导出接口或声明文件中，使用 `~();` 语法声明析构函数。如果省略访问修饰符，则默认为 `internal`。

```cpp
public class Resource {
    public Resource();
    public ~Resource(); // 公开析构函数声明
    
    // ~Resource(); // 如果不加 public，默认为 internal（模块内可见）
}
```

## 4. 成员函数

### 4.1 显式 `self` 参数

C^ 中的成员函数必须显式声明 `self` 参数，用于访问实例。为了简化语法，`self` 关键字本身即代表对当前实例的引用。

| `self` 修饰符 | 描述                             |
| ------------- | -------------------------------- |
| `self`        | 可变引用，可修改实例成员（默认） |
| `self!`       | 不可变引用，不可修改实例成员     |
| `self~`       | 移动语义，获取实例的所有权       |

```cpp
class Person {
    string name_;
    int age_;

    // 可变成员函数（默认 self）
    public func set_name(self, string new_name) -> void {
        self.name_ = new_name;
    }

    // 不可变成员函数（self!）
    public func get_name(self!) -> string {
        // name_ = "test"; // Error: cannot assign to immutable field
        return self.name_;
    }

    // 移动语义成员函数
    public func take_ownership(self~) -> void {
        // 处理 self 的所有权
    }

    // 隐式 self（默认为 self）
    public func greet() -> string {
        return "Hello, my name is " + self.name_;
    }
}
```

### 4.2 不可变成员函数

为了保证不可变对象的线程安全性和逻辑正确性，C^ 引入了类似于 C++ 的 const 成员函数机制，但语法更加统一。

*   **语法**：`func method(self!)`
*   **语义**：在函数体内，`self` 是只读的，无法修改成员变量。

```cpp
class Vector {
    int x;
    int y;

    // 只读方法：self! 表示 self 是不可变的
    public func length(self!) -> float {
        // x = 10; // Error: cannot assign to immutable field
        return sqrt(x*x + y*y);
    }

    // 可变方法：默认 self 是可变的
    public func set(self, int new_x, int new_y) -> void {
        x = new_x;
        y = new_y;
    }
}
```

### 4.3 `mutable` 关键字

在极少数情况下（如缓存、互斥锁），我们需要在不可变成员函数中修改特定成员。C^ 引入 `mutable` 关键字来豁免不可变性检查。

```cpp
class Widget {
    // 逻辑状态
    int size;
    
    // 物理状态（缓存）：标记为 mutable
    mutable int cached_area;
    mutable bool is_area_valid;

    public func area(self!) -> int {
        if (!is_area_valid) {
            // 在 self! 方法中修改 mutable 成员是合法的
            cached_area = size * size;
            is_area_valid = true;
        }
        return cached_area;
    }
}

{
    // 常量引用 self
    func get_name(!self) -> string {
        return self.name;
    }

    // 移动语义 self
    func take_ownership(~self) -> void {
        // 处理 self 的所有权
    }

    // 隐式 self
    func greet() -> string {
        return "Hello, my name is " + name;
    }
}
```

### 4.3 静态成员函数

使用 `static` 关键字声明静态成员函数，静态成员函数属于类而不是实例：

```cpp
class Math {
    // 静态成员函数
    static func add(int a, int b) -> int {
        return a + b;
    }

    static func multiply(int a, int b) -> int {
        return a * b;
    }

    static func pi() -> double {
        return 3.14159265359;
    }
}

// 调用静态成员函数
int sum = Math.add(1, 2);
double pi_value = Math.pi();
```

## 5. 继承

### 5.1 核心设计原则

C^ 采用了**单继承、多接口**的现代面向对象设计模式。

*   **单继承 (Single Inheritance)**：一个类只能继承自一个基类 (Class)。这避免了 C++ 多重继承带来的"菱形继承"数据冗余和复杂的虚基类问题。
*   **多接口 (Multiple Interfaces)**：一个类可以实现任意数量的接口 (Interface)。接口不仅定义契约，还可以提供**默认实现**。

### 5.2 类继承 (Class Inheritance)

#### 5.2.1 基本语法

使用 `:` 指定基类。C^ 仅支持 `public` 继承，且**无需显式书写 `public` 关键字**。

*   **默认即 Public**：继承声明中省略访问修饰符即表示 `public` 继承。
*   **禁止私有继承**：C^ 移除了 C++ 中的 `private`/`protected` 继承，因为这些通常可以用组合（Composition）更好地实现。

```cpp
class Animal {
    string name;

    public Animal(string new_name) {
        name = new_name;
    }

    public func speak() -> string {
        return "Some generic sound";
    }

    public func get_name() -> string {
        return name;
    }
}

// 单继承 (隐式 public)
class Dog : Animal {
    public Dog(string new_name) : base(new_name) {
    }

    public func speak() -> string {
        return "Woof! Woof!";
    }

    public func fetch() -> string {
        return name + " is fetching";
    }
}

// 使用派生类
Dog^ dog = new Dog("Buddy");
string dog_sound = dog.speak(); // "Woof! Woof!"
string dog_fetch = dog.fetch(); // "Buddy is fetching"
string dog_name = dog.get_name(); // "Buddy"（继承自基类）
```

#### 5.2.2 构造与析构

*   **构造**：使用类名 `ClassName(...)`。
*   **析构**：使用 `~ { ... }`。
*   **基类调用**：使用 `: base(...)`。

```cpp
class Derived : Base {
    public Derived(int x) : base(x) {
        // ...
    }

    public ~Derived() {
        // ...
    }
}
```

### 5.3 接口系统 (Interface System)

接口是 C^ 类型系统的核心。接口中不能包含数据成员（Field），但可以包含：
1.  抽象方法声明。
2.  **默认实现方法** (Default Implementation)。
3.  静态方法。

#### 5.3.1 定义与实现

```cpp
interface ILogger {
    // 抽象方法
    public func log(string msg);

    // 默认实现
    public func log_error(string msg) {
        self.log("[ERROR] " + msg);
    }
}

interface ISerializable {
    public func to_json() -> string;
}

// 实现多个接口
class FileLogger : ILogger, ISerializable {
    public func log(string msg) {
        // write to file...
    }

    public func to_json() -> string {
        return "{}";
    }

    // log_error 直接使用接口的默认实现，无需重写
}
```

### 5.4 接口冲突与仲裁 (Conflict Resolution)

当一个类实现多个接口，且这些接口包含**同名且参数列表相同**的方法（无论是默认实现还是抽象声明）时，编译器会报错，要求开发者显式解决冲突。

C^ 提供两种解决方案：**仲裁 (Arbitration)** 和 **重写 (Override)**。

#### 5.4.1 场景假设

```cpp
interface IWifi {
    public func connect() {
        print("Connecting via WiFi...");
    }
}

interface IBluetooth {
    public func connect() {
        print("Connecting via Bluetooth...");
    }
}
```

#### 5.4.2 方案 A：仲裁 (Arbitration)

如果开发者希望直接复用其中某一个接口的逻辑，可以使用 `using` 关键字指定。

```cpp
class SmartDevice : IWifi, IBluetooth {
    // 仲裁：明确指定 connect() 使用 IWifi 的实现
    public using IWifi::connect; 
}

// 使用
SmartDevice dev;
dev.connect(); // 输出: Connecting via WiFi...
```

#### 5.4.3 方案 B：显式重写 (Explicit Override)

如果两个接口的逻辑都不适用，或者需要组合它们的逻辑，开发者必须在类中显式重写该方法。

```cpp
class DualModeDevice : IWifi, IBluetooth {
    public func connect() {
        print("Checking signal strength...");
        
        // 可以选择性调用特定接口的实现
        if (wifi_strong) {
            IWifi::connect();
        } else {
            IBluetooth::connect();
        }
    }
}
```

#### 5.4.4 方案 C：使用组合 (Composition) - **推荐**

如果 `IWifi` 和 `IBluetooth` 的 `connect` 逻辑完全不同，且需要在同一个对象中并存，我们建议使用**组合**而非继承。这符合 "Composition over Inheritance" 的原则，也能提供更清晰的 API。

```cpp
class HybridDevice {
    private WifiModule _wifi;
    private BluetoothModule _bt;

    public func connect_wifi() {
        _wifi.connect();
    }

    public func connect_bluetooth() {
        _bt.connect();
    }
}
```

> **设计决策**：C^ 移除了 C# 风格的 "显式接口实现" (Explicit Interface Implementation)。我们认为，一个对象在被转换为不同接口时表现出完全不同的行为（且方法名相同），是一种容易导致混淆的 "Magic" 行为。如果行为不同，应当体现为不同的方法名或不同的组件对象。

### 5.5 继承 vs 组合

虽然 C^ 提供了强大的接口继承能力，但我们依然坚持 **"组合优于继承" (Composition over Inheritance)** 的原则。

*   **继承**：适用于严格的 `is-a` 关系，且主要为了复用接口契约。
*   **组合**：适用于 `has-a` 关系，主要为了复用实现逻辑。

```cpp
// 推荐：使用组合复用逻辑
class Browser {
    private Downloader _downloader; // 组合
    private Renderer _renderer;     // 组合
}
```

### 5.6 继承访问控制

| 基类成员访问修饰符 | 在派生类中的可见性 |
| ------------------ | ------------------ |
| `public`           | `public`           |
| `protected`        | `protected`        |
| `private`          | 不可见             |

## 6. 多态

### 6.1 方法重写

派生类可以重写基类的方法，实现多态：

```cpp
class Animal {
    func speak() -> string {
        return "Some generic sound";
    }
}

class Dog : Animal {
    func speak() -> string {
        return "Woof! Woof!";
    }
}

class Cat : Animal {
    func speak() -> string {
        return "Meow! Meow!";
    }
}

// 多态
Animal^ animal1 = new Dog();
Animal^ animal2 = new Cat();

string sound1 = animal1.speak(); // "Woof! Woof!"
string sound2 = animal2.speak(); // "Meow! Meow!"
```

### 6.2 抽象类与接口

#### 6.2.1 抽象类

抽象类是不能直接实例化的类，用于定义子类的共同接口：

```cpp
// 抽象类
abstract class Animal {
    // 抽象方法（无实现）
    abstract func speak() -> string;

    // 普通方法（有实现）
    func breathe() -> string {
        return "Breathing...";
    }
}

// 派生自抽象类
class Dog : Animal {
    // 实现抽象方法
    func speak() -> string {
        return "Woof! Woof!";
    }
}

// 错误：不能实例化抽象类
// Animal^ animal = new Animal();

// 正确：实例化派生类
Dog^ dog = new Dog();
string sound = dog.speak(); // "Woof! Woof!"
string breathing = dog.breathe(); // "Breathing..."
```

#### 6.2.2 接口

接口是一种特殊的抽象类，只包含抽象方法和常量：

```cpp
// 接口
interface Drawable {
    func draw() -> void;
}

interface Resizable {
    func resize(int width, int height) -> void;
}

// 实现接口
class Rectangle : Drawable, Resizable {
    int width;
    int height;

    public Rectangle(int width, int height) {
        self.width = width;
        self.height = height;
    }

    // 实现 Drawable 接口
    func draw() -> void {
        println("Drawing rectangle with width " + width + " and height " + height);
    }

    // 实现 Resizable 接口
    func resize(int new_width, int new_height) -> void {
        width = new_width;
        height = new_height;
    }
}

// 使用接口
Drawable^ drawable = new Rectangle(10, 20);
drawable->draw(); // 使用 -> 运算符访问指针成员
// 也可以使用解引用运算符 ^ 然后用 . 访问成员
drawable^.draw(); // "Drawing rectangle with width 10 and height 20"

Resizable^ resizable = new Rectangle(10, 20);
resizable->resize(30, 40); // 使用 -> 运算符访问指针成员
// 也可以使用解引用运算符 ^ 然后用 . 访问成员
resizable^.resize(30, 40);
```

## 7. 属性

### 7.1 基本属性

属性是一种特殊的成员，提供了对字段的封装访问：

```cpp
class Person {
    // 私有字段
    private string _name;
    private int _age;

    // 名称属性
    public func get name() -> string {
        return _name;
    }

    public func set name(string value) {
        _name = value;
    }

    // 年龄属性
    public func get age() -> int {
        return _age;
    }

    public func set age(int value) {
        if (value >= 0) {
            _age = value;
        }
    }

    public Person(string name, int age) {
        _name = name;
        _age = age;
    }
}

// 使用属性
Person^ person = new Person("Alice", 30);
string name = person.name; // 调用 get name()
person.age = 31; // 调用 set age(int value)
person.age = -1; // 无效，不会设置
```

### 7.2 只读属性

如果只提供 `get` 访问器，属性就是只读的：

```cpp
class Circle {
    private double _radius;

    // 只读属性
    public func get radius() -> double {
        return _radius;
    }

    // 计算属性（只读）
    public func get area() -> double {
        return 3.14159 * _radius * _radius;
    }

    public Circle(double radius) {
        _radius = radius;
    }
}

Circle^ circle = new Circle(5.0);
double r = circle.radius; // 5.0
double a = circle.area; // 78.53975

// 错误：不能设置只读属性
// circle.radius = 10.0;
// circle.area = 100.0;
```

### 7.3 自动属性

C^ 支持自动属性，编译器会自动生成后备字段：

```cpp
class Person {
    // 自动属性
    public string name;
    public int age;

    public Person(string name, int age) {
        self.name = name;
        self.age = age;
    }
}

// 使用自动属性
Person^ person = new Person("Alice", 30);
string name = person.name;
person.age = 31;
```

## 8. 运算符重载

C^ 支持运算符重载，允许为自定义类型定义运算符行为：

```cpp
class Vector2D {
    public int x;
    public int y;

    public Vector2D(int x, int y) {
        self.x = x;
        self.y = y;
    }

    // 重载 + 运算符
    func operator+(Vector2D other) -> Vector2D {
        return new Vector2D(x + other.x, y + other.y);
    }

    // 重载 - 运算符
    func operator-(Vector2D other) -> Vector2D {
        return new Vector2D(x - other.x, y - other.y);
    }

    // 重载 * 运算符（标量乘法）
    func operator*(int scalar) -> Vector2D {
        return new Vector2D(x * scalar, y * scalar);
    }

    // 重载 == 运算符
    func operator==(Vector2D other) -> bool {
        return x == other.x && y == other.y;
    }

    // 重载 != 运算符
    func operator!=(Vector2D other) -> bool {
        return !(self == other);
    }
}

// 使用重载的运算符
Vector2D^ v1 = new Vector2D(1, 2);
Vector2D^ v2 = new Vector2D(3, 4);

Vector2D^ sum = v1 + v2; // (4, 6)
Vector2D^ difference = v1 - v2; // (-2, -2)
Vector2D^ scaled = v1 * 2; // (2, 4)

bool equal = (v1 == v2); // false
bool not_equal = (v1 != v2); // true
```

## 9. 索引器

C^ 支持索引器，允许使用数组语法访问对象：

```cpp
class MyCollection {
    private int[] _items;
    private int _count;

    public MyCollection(int capacity) {
        _items = new int[capacity];
        _count = 0;
    }

    // 索引器
    public func get(int index) -> int {
        if (index >= 0 && index < _count) {
            return _items[index];
        }
        throw IndexOutOfRangeException();
    }

    public func set(int index, int value) {
        if (index >= 0 && index < _count) {
            _items[index] = value;
        }
        throw IndexOutOfRangeException();
    }

    public func add(int value) -> void {
        if (_count < _items.length) {
            _items[_count] = value;
            _count++;
        }
    }

    public func get count() -> int {
        return _count;
    }
}

// 使用索引器
MyCollection^ collection = new MyCollection(10);
collection.add(1);
collection.add(2);
collection.add(3);

int first = collection[0]; // 1
int second = collection[1]; // 2
collection[2] = 4; // 修改第三个元素
int third = collection[2]; // 4
```



## 10. 泛型类

C^ 支持泛型类，用于创建类型安全的容器和算法：

```cpp
// 泛型类
class List<T> {
    private T[] _items;
    private int _count;

    public List(int capacity) {
        _items = new T[capacity];
        _count = 0;
    }

    public func add(T item) -> void {
        if (_count < _items.length) {
            _items[_count] = item;
            _count++;
        }
    }

    public func get(int index) -> T {
        if (index >= 0 && index < _count) {
            return _items[index];
        }
        throw IndexOutOfRangeException();
    }

    public func get count() -> int {
        return _count;
    }
}

// 使用泛型类
List<int>^ int_list = new List<int>(10);
int_list.add(1);
int_list.add(2);
int value = int_list.get(0); // 1

List<string>^ string_list = new List<string>(10);
string_list.add("Hello");
string_list.add("World");
string text = string_list.get(1); // "World"
```

### 10.1 泛型约束

使用 `where` 子句为泛型参数添加约束：

```cpp
// 带约束的泛型类
class Repository<T> where T : IComparable, new() {
    private List<T>^ _items;

    public Repository() {
        _items = new List<T>(10);
    }

    public func add(T item) -> void {
        _items.add(item);
    }

    public func find(T item) -> int {
        for (int i = 0; i < _items.count; i++) {
            if (_items.get(i) == item) {
                return i;
            }
        }
        return -1;
    }

    public func create() -> T {
        return new T();
    }
}
```

## 11. 静态成员
 
 ### 11.1 静态字段
 
 静态字段属于类而不是实例，所有实例共享同一个静态字段：
 
 ```cpp
 class Counter {
     // 静态字段
     private static int _count;
 
     // 静态构造函数
     static func init() {
         _count = 0;
     }
 
     // 静态属性
     public static func get count() -> int {
         return _count;
     }
 
     // 静态方法
     public static func increment() -> void {
         _count++;
     }
 
     // 实例构造函数
     func init() {
         Counter.increment();
     }
 }
 
 // 使用静态成员
 Counter^ c1 = new Counter();
 Counter^ c2 = new Counter();
 Counter^ c3 = new Counter();
 
 int total = Counter.count; // 3
 ```
 
 ### 11.2 静态构造函数

静态构造函数用于初始化静态成员，在类第一次使用时自动调用：

```cpp
class Config {
    public static string app_name;
    public static int version;

    // 静态构造函数
    static func init() {
        app_name = "My Application";
        version = 1;
        println("Static constructor called");
    }
}

// 第一次使用类时，静态构造函数会被调用
string name = Config.app_name; // "My Application"
int ver = Config.version; // 1
```

## 12. 密封类

使用 `sealed` 关键字声明密封类，密封类不能被继承：

```cpp
// 密封类
sealed class StringUtils {
    public static func is_empty(string s) -> bool {
        return s == null || s.length == 0;
    }

    public static func trim(string s) -> string {
        // 实现 trim 逻辑
        return s;
    }
}

// 错误：不能继承密封类
// class MyStringUtils : StringUtils {
// }

// 使用密封类
bool empty = StringUtils.is_empty(""); // true
```

## 13. 部分类

C^ 支持部分类，允许将一个类的定义分散到多个文件中：

```cpp
// 文件1: Person.part1.ch
partial class Person {
    private string _name;
    private int _age;

    func init(string name, int age) {
        _name = name;
        _age = age;
    }
}

// 文件2: Person.part2.ch
partial class Person {
    public func get name() -> string {
        return _name;
    }

    public func get age() -> int {
        return _age;
    }

    public func greet() -> string {
        return "Hello, my name is " + _name;
    }
}

// 使用部分类
Person^ person = new Person("Alice", 30);
string greeting = person.greet(); // "Hello, my name is Alice"
```

## 14. 最佳实践

### 14.1 类设计

1. **单一职责原则**：每个类应该只负责一个功能领域
2. **封装**：使用私有字段和公开属性，隐藏实现细节
3. **继承层次**：保持继承层次简洁，避免过深的继承树
4. **接口隔离**：使用小而专注的接口，而不是大而全的接口
5. **依赖倒置**：依赖抽象而不是具体实现

### 14.2 命名约定

1. **类名**：使用 PascalCase，如 `Person`、`StringUtils`
2. **方法名**：使用 PascalCase，如 `GetName`、`CalculateTotal`
3. **字段名**：使用 camelCase，私有字段可加下划线前缀，如 `_name`、`_age`
4. **属性名**：使用 PascalCase，如 `Name`、`Age`
5. **参数名**：使用 camelCase，如 `name`、`age`

### 14.3 性能考虑

1. **避免过度继承**：深层继承会增加方法调用开销
2. **合理使用虚方法**：虚方法调用比非虚方法慢
3. **使用值类型**：对于小的、不可变的数据，考虑使用值类型
4. **避免频繁创建对象**：使用对象池或缓存减少GC压力
5. **优化方法调用**：对于频繁调用的方法，考虑内联

## 15. 总结

C^ 语言提供了完整的面向对象编程支持，包括：

- **类**：封装数据和行为
- **继承**：创建层次化的类结构
- **多态**：通过方法重写实现运行时多态
- **抽象**：使用抽象类和接口定义契约
- **封装**：通过访问修饰符控制成员可见性
- **属性**：提供对字段的封装访问
- **运算符重载**：为自定义类型定义运算符行为
- **索引器**：允许使用数组语法访问对象
- **泛型**：创建类型安全的容器和算法
- **静态成员**：属于类而不是实例的成员
- **密封类**：防止被继承
- **部分类**：将类定义分散到多个文件

通过合理使用这些特性，可以编写清晰、可维护、可扩展的面向对象代码。C^ 的面向对象特性设计兼顾了表达能力和性能，为系统编程提供了现代化的面向对象编程体验。