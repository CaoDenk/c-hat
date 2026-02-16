# C^ 语言语法规范

本文档详细介绍 C^ 语言的语法规范，包括词法元素、语法结构、语句、表达式等内容，为编译器开发提供清晰的语法参考。

## 1. 词法元素

### 1.1 关键字

C^ 语言的保留关键字如下（按功能分类）：

#### 声明与定义
| 关键字   | 说明                        |
| :------- | :-------------------------- |
| `func`   | 函数声明                    |
| `class`  | 类声明                      |
| `struct` | 结构体                      |
| `enum`   | 枚举                        |
| `union`  | 联合体                      |
| `import` | 导入模块                    |
| `module` | 模块声明                    |
| `var`    | 可变变量声明                |
| `let`    | 不可变变量声明 (运行时只读) |
| `const`  | 编译期常量声明              |
| `late`   | 延迟初始化声明              |
| `using`  | 使用声明                    |
| `extern` | 外部声明                    |

#### 控制流
| 关键字       | 说明                   |
| :----------- | :--------------------- |
| `if`, `else` | 条件分支               |
| `match`      | 模式匹配 (取代 switch) |
| `for`        | 循环                   |
| `while`      | 循环                   |
| `do`         | 循环                   |
| `break`      | 跳出                   |
| `continue`   | 继续                   |
| `return`     | 返回                   |
| `goto`       | 跳转                   |
| `try`        | 异常处理/Result传播    |
| `catch`      | 异常捕获               |
| `throw`      | 抛出异常               |
| `defer`      | 延迟执行 (资源清理)    |
| `await`      | 等待协程               |
| `yield`      | 协程产出               |

#### 类型系统
| 关键字            | 说明                    |
| :---------------- | :---------------------- |
| `void`            | 空类型                  |
| `bool`            | 布尔类型                |
| `byte`, `sbyte`   | 8位整数 (无符号/有符号) |
| `short`, `ushort` | 16位整数                |
| `int`, `uint`     | 32位整数                |
| `long`, `ulong`   | 64位整数                |
| `float`, `double` | 32/64位浮点数           |
| `fp16`, `bf16`    | 16位半精度/脑浮点数     |
| `char`            | 32位 Unicode 字符       |
| `string`          | UTF-8 字符串            |
| `string_view`     | 字符串视图              |
| `true`, `false`   | 布尔字面量              |
| `null`            | 空指针字面量            |

#### 修饰符与操作符
| 关键字      | 说明                          |
| :---------- | :---------------------------- |
| `public`    | 公开/导出                     |
| `private`   | 文件私有                      |
| `protected` | 继承可见                      |
| `internal`  | 模块可见 (默认)               |
| `static`    | 静态成员                      |
| `virtual`   | 虚函数                        |
| `override`  | 重写                          |
| `abstract`  | 抽象                          |
| `final`     | 禁止继承/重写                 |
| `mutable`   | 可变修饰符 (Lambda/Const成员) |
| `self`      | 当前实例引用                  |
| `base`      | 基类引用                      |
| `new`       | 堆分配                        |
| `delete`    | 堆释放                        |
| `sizeof`    | 大小查询                      |
| `typeof`    | 类型查询                      |
| `as`        | 类型转换                      |
| `is`        | 类型检查                      |
| `where`     | 泛型约束子句                  |
| `requires`  | 结构化约束表达式              |
| `concept`   | 约束命名 (编译期)             |
| `operator`  | 运算符重载                    |
| `comptime`  | 编译期执行                    |

> **注**：`this` 已移除，统一使用 `self`。`switch` 已移除，统一使用 `match`。`export` 已移除，使用 `public`。`case` 和 `default` 不是关键字，match 表达式使用 `=>` 和 `_`。`namespace` 已移除，命名空间由 `module` 声明决定。

### 1.2 标识符

标识符是用来命名变量、函数、类等实体的名称，遵循以下规则：

- 以字母、下划线或美元符号开头
- 后续字符可以是字母、数字、下划线或美元符号
- 区分大小写
- 不能是关键字

示例：

```cpp
int x = 10;      // 合法标识符
string _name = "test";  // 合法标识符
double $value = 3.14;  // 合法标识符
int 123abc = 0;  // 非法标识符（以数字开头）
int if = 0;      // 非法标识符（是关键字）
```

### 1.3 字面量

#### 1.3.1 整数字面量

整数字面量可以是十进制、十六进制或二进制（不支持八进制）：

- 十进制：直接书写数字，如 `123`
- 十六进制：以 `0x` 或 `0X` 开头，如 `0x1A`
- 二进制：以 `0b` 或 `0B` 开头，如 `0b1010`

整数字面量可以添加下划线作为分隔符，如 `1_000_000`。

支持类型后缀：
- `u` / `U`: 无符号
- `l` / `L`: 长整型 (64位)
- `ul`, `uL`, `Ul`, `UL`...: 无符号长整型
- `y` / `Y`: byte (8位)
- `s` / `S`: short (16位)

#### 1.3.2 浮点字面量

浮点字面量可以是小数形式或指数形式：

- 小数形式：如 `3.14`、`0.5`
- 指数形式：如 `1e10`、`2.5e-3`

支持类型后缀：
- `f` / `F`: float (32位)
- `d` / `D`: double (64位) - 默认
- `h` / `H`: fp16 (16位半精度)
- `bf`: bf16 (16位 Brain Float)

#### 1.3.3 字符字面量

字符字面量用单引号包围，如 `'a'`。默认类型为 `char` (32位 Unicode 码点)。

前缀支持：
- `u8'a'`: byte (8位)
- `u16'a'`: char16 (16位)
- `u32'a'`: char32 (32位)

支持的转义序列：

| 转义序列     | 含义              |
| ------------ | ----------------- |
| `\n`         | 换行符            |
| `\t`         | 制表符            |
| `\r`         | 回车符            |
| `\\`         | 反斜杠            |
| `\'`         | 单引号            |
| `\"`         | 双引号            |
| `\0`         | 空字符            |
| `\uXXXX`     | 16位 Unicode 转义 |
| `\UXXXXXXXX` | 32位 Unicode 转义 |

#### 1.3.4 字符串字面量

字符串字面量用双引号包围，如 `"hello"`。默认类型为 `std.literal_view` (UTF-8 编码，指向静态内存)。

前缀支持：
- `u16"..."`: UTF-16 字符串
- `u32"..."`: UTF-32 字符串
- `b"..."`: 二进制字节串 (不校验编码)
- `$"..."`: 标准库 `string` 对象 (支持插值)
- `#"..."#`: 原始字符串 (Raw String，不转义)
- `identifier"..."`: 用户自定义前缀 (如 `regex"..."`)
- `$#` / `$"""`: 组合前缀

多行字符串使用 `""" ... """`。

**注意**：`std.literal_view` 是一个标记类型，保证指向的数据拥有静态生命周期。可以隐式转换为 `string` 类型。

#### 1.3.5 布尔字面量

布尔字面量有两个值：`true` 和 `false`。

#### 1.3.6 空字面量

空字面量为 `null`，表示空指针或空引用。

### 1.4 运算符

#### 1.4.1 算术运算符

| 运算符 | 含义             |
| ------ | ---------------- |
| `+`    | 加法             |
| `-`    | 减法             |
| `*`    | 乘法             |
| `/`    | 除法             |
| `%`    | 取模             |
| `**`   | 幂运算           |
| `++`   | 自增 (前缀/后缀) |
| `--`   | 自减 (前缀/后缀) |

#### 1.4.2 赋值运算符

| 运算符 | 含义         |
| ------ | ------------ |
| `=`    | 赋值         |
| `+=`   | 加法赋值     |
| `-=`   | 减法赋值     |
| `*=`   | 乘法赋值     |
| `/=`   | 除法赋值     |
| `%=`   | 取模赋值     |
| `&=`   | 按位与赋值   |
| `      | =`           | 按位或赋值 |
| `^=`   | 按位异或赋值 |
| `<<=`  | 左移赋值     |
| `>>=`  | 右移赋值     |

#### 1.4.3 比较运算符

| 运算符 | 含义     |
| ------ | -------- |
| `==`   | 等于     |
| `!=`   | 不等于   |
| `<`    | 小于     |
| `<=`   | 小于等于 |
| `>`    | 大于     |
| `>=`   | 大于等于 |
| `is`   | 类型检查 |
| `as`   | 类型转换 |

#### 1.4.4 逻辑运算符

| 运算符 | 含义   |
| ------ | ------ |
| `&&`   | 逻辑与 |
| `      |        | ` | 逻辑或 |
| `!`    | 逻辑非 |

#### 1.4.5 位运算符

| 运算符 | 含义     |
| ------ | -------- |
| `&`    | 按位与   |
| `      | `        | 按位或 |
| `^`    | 按位异或 |
| `~`    | 按位取反 |
| `<<`   | 左移     |
| `>>`   | 右移     |

#### 1.4.6 其他运算符

| 运算符 | 含义                          |
| ------ | ----------------------------- |
| `.`    | 成员访问                      |
| `->`   | 指针成员访问                  |
| `[]`   | 索引访问                      |
| `()`   | 函数调用                      |
| `?:`   | 三元条件                      |
| `??`   | 空合并 (Null Coalescing)      |
| `..`   | 范围操作符                    |
| `=>`   | Lambda/Match 箭头             |
| `^`    | 前缀: 取地址 / 后缀: 指针类型 |
| `!`    | 后缀: 不可变修饰符            |
| `~`    | 后缀: 移动语义                |
| `$`    | 前缀: 倒序索引                |
| `@`    | 反射操作符                    |
| `...`  | 可变参数                      |
| `:`    | 类型标注                      |

### 1.5 分隔符

| 分隔符 | 含义     |
| ------ | -------- |
| `(`    | 左括号   |
| `)`    | 右括号   |
| `{`    | 左大括号 |
| `}`    | 右大括号 |
| `[`    | 左中括号 |
| `]`    | 右中括号 |
| `;`    | 分号     |
| `,`    | 逗号     |
| `:`    | 冒号     |
| `.`    | 点       |
| `->`   | 箭头     |
| `=>`   | 胖箭头   |
| `#`    | 哈希符号 |

## 2. 语法结构

### 2.1 程序结构

C^ 语言程序由一系列声明组成，包括变量声明、函数声明、类声明等。

示例：

```cpp
import std.io;

func main() -> int {
  int x = 10;
  int y = 20;
  print(x + y);
  return 0;
}
```

### 2.2 声明

#### 2.2.1 变量声明

变量声明使用 `var`、`let` 或 `late` 关键字：

- 显式类型声明：`Type variable_name = value;`
- `var`：声明可变变量，类型推导：`var variable_name = value;`
- `let`：声明不可变变量，类型推导：`let variable_name = value;`
- `late`：声明延迟初始化变量：`late Type variable_name;`

`late` 是 C^ 的核心优化特性，用于在不确定是否初始化的情况下预留栈空间，解决异常安全和条件初始化难题。

*   **栈上预留**：`late T var;` 在栈帧上分配 `sizeof(T)` 空间，但不调用构造函数。
*   **RVO 配合**：在 `try-catch` 或条件分支中赋值时，触发 RVO（返回值优化），直接在预留空间构造，避免临时对象拷贝。
*   **状态追踪**：编译器会追踪 `late` 变量的初始化状态。

语法：

```cpp
// 显式类型声明
Type variable_name = value;

// 类型推导，可变变量
var variable_name = value;

// 类型推导，不可变变量
let variable_name = value;

// 延迟初始化变量
late Type variable_name;
```

示例：

```cpp
// 显式类型声明
int counter = 0;
byte mask = 0xFF;
char ch = '中';

// 类型推导，可变变量
var total = 100;
var name = "C-Hat";

// 类型推导，不可变变量
let limit = 50;

// 延迟初始化变量
late DatabaseConnection db;
db = connect_to_db("localhost")~;

// 延迟初始化的高级用法
public func process_data() {
  late DatabaseConnection db; // 仅分配内存，不连接
  
  try {
    // 在 try 块内初始化
    db = connect_to_db("localhost")~; 
    // 此时 db 已构造，可以直接使用
    db.query("SELECT * FROM users");
  } catch (NetworkError! e) {
    // 若初始化失败，db 仍处于未构造状态，析构函数不会被调用
    print("Connection failed");
    return;
  }
  
  // 出了 try 块，db 依然有效（如果在 try 中成功初始化）
  // 编译器确保此时 db 是已初始化的，或者是不可达的
}
```

#### 2.2.2 函数声明

函数是逻辑的核心。C^ 使用 `func` 关键字，既消除了 C++ 函数声明解析的歧义（Most Vexing Parse），又保持了定义的优雅。

*   **关键字**：统一使用 `func`。
*   **参数**：`Type name` 格式，与变量声明一致。
*   **返回值**：使用 `-> Type` 后置语法，支持元组多返回值。
*   **简写**：单表达式函数可使用 `=>`。

语法：

```cpp
// 标准函数定义
func identifier(parameter_type parameter_name, ...) -> return_type {
  // 函数体
  return expression;
}

// 单表达式函数
func identifier(parameter_type parameter_name, ...) -> return_type => expression;

// 无返回值函数
func identifier(parameter_type parameter_name, ...) {
  // 函数体
}

// 泛型函数
func <T> identifier(T parameter_name, ...) -> return_type {
  // 函数体
  return expression;
}

// 带约束的泛型
func <T> identifier(T parameter_name, ...) -> return_type where Constraint<T> {
  // 函数体
  return expression;
}

// 多返回值函数
func identifier(parameter_type parameter_name, ...) -> (return_type1, return_type2, ...) {
  // 函数体
  return (expression1, expression2, ...);
}
```

示例：

```cpp
// 标准函数定义
func calculate_area(int width, int height) -> int {
  if (width < 0 || height < 0) {
    return 0;
  }
  return width * height;
}

// 单表达式函数 (Lambda-like syntax)
func add(int a, int b) -> int => a + b;

// 多返回值
func get_status() -> (int, string) {
  return (200, "OK");
}

// 带约束的泛型函数
func <T> max(T a, T b) -> T where Comparable<T> {
  return a > b ? a : b;
}

// 调用
int w = 10;
int h = 20;
int area = calculate_area(w, h);
(int code, string msg) = get_status();
```

#### 2.2.3 类声明

C^ 支持面向对象编程，同时保持函数作为一等公民的地位。类提供了封装、继承和多态的能力。

*   **一等公民函数**：摒弃了 Java/C# 的强行类包裹。
*   **显式参数**：成员函数必须显式声明 `self` 及其修饰符（`&` 改、`!` 读、`~` 移）。
*   **属性**：支持属性语法，提供 getter 和 setter。

语法：

```cpp
[access_modifier] class identifier [: base_class] {
  // 成员变量
  [access_modifier] Type member_name;
  
  // 属性
  [access_modifier] Type property_name {
    get => expression;
    set { statement; }
  }
  
  // 构造函数
  [access_modifier] func init(parameter_type parameter_name, ...) {
    // 构造函数体
  }
  
  // 成员函数
  [access_modifier] func method_name(self, parameter_type parameter_name, ...) -> return_type {
    // 函数体
    return expression;
  }
}
```

示例：

```cpp
public class Player {
  private int _health;
  
  // 属性 (Property)
  public int health {
    get => field;
    set { field = value > 100 ? 100 : value; }
  }

  // 成员函数
  public func take_damage(self, int amount) {
    self._health -= amount;
  }
  
  // 构造函数
  public func init(int initial_health) {
    self._health = initial_health;
  }
}

// 继承
public class Boss : Player {
  private int _damage_multiplier;
  
  public func init(int initial_health, int damage_multiplier) {
    base.init(initial_health);
    self._damage_multiplier = damage_multiplier;
  }
  
  public func attack(self, Player& target) {
    target.take_damage(10 * self._damage_multiplier);
  }
}
```

#### 2.2.4 结构体声明

结构体声明使用 `struct` 关键字：

语法：

```cpp
struct identifier {
  // 成员变量
  // 成员函数
}
```

示例：

```cpp
struct Point {
  int x;
  int y;
  
  func distance() -> double {
    return sqrt(x * x + y * y);
  }
}
```

#### 2.2.5 枚举声明

枚举声明使用 `enum` 关键字：

语法：

```cpp
enum identifier {
  identifier [= value],
  identifier [= value],
  // ...
}
```

示例：

```cpp
enum Color {
  Red,
  Green,
  Blue = 4,
  Yellow
}
```

#### 2.2.6 命名空间声明 (已移除)

C^ 移除了显式的 `namespace` 关键字。命名空间完全由 `module` 声明决定。

示例：

```cpp
// 以前的写法 (已不支持)
// namespace Math { ... }

// 现在的写法
// 在 utils/math.ch 中:
module utils.math; 

func add(...) { ... } // 自动位于 utils::math::add
```

### 2.3 语句

#### 2.3.1 表达式语句

表达式语句是一个表达式后面跟着分号：

语法：

```cpp
expression;
```

示例：

```cpp
x = 10;
print("hello");
add(5, 3);
```

#### 2.3.2 复合语句

复合语句是由大括号包围的一系列语句：

语法：

```cpp
{
  // 语句
}
```

示例：

```cpp
{
  int x = 10;
  int y = 20;
  print(x + y);
}
```

#### 2.3.3 条件语句

条件语句使用 `if` 和 `else` 关键字：

语法：

```cpp
if (condition) {
  // 语句
} else if (condition) {
  // 语句
} else {
  // 语句
}
```

示例：

```cpp
if (x > 0) {
  print("x is positive");
} else if (x < 0) {
  print("x is negative");
} else {
  print("x is zero");
}
```

#### 2.3.4 Match 表达式

C^ 的控制流旨在消除 C/C++ 中的常见陷阱（如悬挂 else、switch 穿透），提供更理性的分支处理。Match 表达式取代了传统的 switch 语句，支持模式匹配，强制穷举，无隐式 Fallthrough。

*   **关键字**：使用 `match` 关键字。
*   **模式匹配**：支持常量模式、变量模式、通配符模式等。
*   **强制穷举**：所有可能的情况都必须被覆盖，或提供默认分支。
*   **无隐式 Fallthrough**：匹配成功后自动终止，无需 break 语句。

语法：

```cpp
match (expression) {
  pattern => expression,
  pattern1, pattern2 => expression, // 多个模式匹配同一个表达式
  _ => expression // 默认分支（必选，除非所有情况都被覆盖）
}
```

示例：

```cpp
// 基本 match 表达式
string status_text = match (code) {
  200 => "OK",
  404 => "Not Found",
  500, 502 => "Server Error", // 多个匹配
  _   => "Unknown"            // 默认分支 (必选)
};

// 变量模式
match (value) {
  let x if x > 0 => print("Positive: " + x),
  let x if x < 0 => print("Negative: " + x),
  _ => print("Zero")
};
```

#### 2.3.5 循环语句

##### 2.3.5.1 for 循环

语法：

```cpp
for (initialization; condition; update) {
  // 语句
}
```

示例：

```cpp
for (int i = 0; i < 10; i++) {
  print(i);
}

// 或使用类型推导
for (var i = 0; i < 10; i++) {
  print(i);
}
```

##### 2.3.5.2 while 循环

语法：

```cpp
while (condition) {
  // 语句
}
```

示例：

```cpp
int i = 0;
while (i < 10) {
  print(i);
  i++;
}
```

##### 2.3.5.3 do-while 循环

语法：

```cpp
do {
  // 语句
} while (condition);
```

示例：

```cpp
int i = 0;
do {
  print(i);
  i++;
} while (i < 10);
```

##### 2.3.5.4 foreach 循环

语法：

```cpp
foreach (identifier in expression) {
  // 语句
}
```

示例：

```cpp
int[] arr = [1, 2, 3, 4, 5];
foreach (item in arr) {
  print(item);
}

foreach (i, item in arr) {
  print("Index " + i + ": " + item);
}
```

#### 2.3.6 跳转语句

##### 2.3.6.1 break 语句

`break` 语句用于跳出循环或开关语句：

```cpp
for (int i = 0; i < 10; i++) {
  if (i == 5) {
    break;
  }
  print(i);
}
```

##### 2.3.6.2 continue 语句

`continue` 语句用于跳过当前循环的剩余部分，进入下一次循环：

```cpp
for (int i = 0; i < 10; i++) {
  if (i % 2 == 0) {
    continue;
  }
  print(i);
}
```

##### 2.3.6.3 return 语句

`return` 语句用于从函数返回值：

```cpp
func add(int x, int y) -> int {
  return x + y;
}
```

#### 2.3.7 异常处理语句

##### 2.3.7.1 try-catch 语句

`try-catch` 语句用于处理异常：

语法：

```cpp
try {
  // 可能抛出异常的代码
} catch (type identifier) {
  // 异常处理代码
} catch (type identifier) {
  // 异常处理代码
}
```

示例：

```cpp
try {
  int result = divide(10, 0);
} catch (DivideByZeroError e) {
  print("Divide by zero error");
} catch (Exception e) {
  print("General exception: " + e.message);
}
```

##### 2.3.7.2 defer 语句

`defer` 语句用于延迟执行代码，直到当前作用域结束：

语法：

```cpp
defer expression;
```

示例：

```cpp
func read_file(string path) -> string {
  var file = open_file(path);
  defer close_file(file);
  
  // 读取文件内容
  var content = file.read();
  return content;
  // 这里会自动执行 close_file(file)
}
```

#### 2.3.8 协程语句

##### 2.3.8.1 await 表达式

`await` 关键字用于等待异步操作完成：

```cpp
func main() -> int {
  var data = await fetch_data("https://example.com");
  print(data);
  return 0;
}
```

##### 2.3.8.3 yield 语句

`yield` 语句用于从协程中产出值：

```cpp
func generate_values<T>(T[] values) -> T {
  foreach (value in values) {
    yield value;
  }
}

func main() -> int {
  var generator = generate_values([1, 2, 3, 4, 5]);
  foreach (value in generator) {
    print(value);
  }
  return 0;
}
```

#### 2.3.9 编译时语句

`comptime` 关键字用于在编译时执行代码：

```cpp
comptime {
  // 编译时执行的代码
  int x = 10;
  int y = 20;
  const Z = x + y;
}

func main() -> int {
  print(Z); // 编译时计算的结果
  return 0;
}
```

### 2.4 表达式

#### 2.4.1 基本表达式

- 字面量：`10`, `3.14`, `"hello"`, `true`
- 标识符：`x`, `name`, `compute_value`
- 括号表达式：`(x + y) * z`

#### 2.4.2 一元表达式

- 前缀一元表达式：`-x`, `!flag`, `&x`, `^x`
- 后缀一元表达式：`x++`, `x--`

#### 2.4.3 二元表达式

- 算术表达式：`x + y`, `x - y`, `x * y`, `x / y`, `x % y`
- 比较表达式：`x == y`, `x != y`, `x < y`, `x <= y`, `x > y`, `x >= y`
- 逻辑表达式：`x && y`, `x || y`
- 位表达式：`x & y`, `x | y`, `x ^ y`, `x << y`, `x >> y`
- 赋值表达式：`x = y`, `x += y`, `x -= y`, `x *= y`, `x /= y`

#### 2.4.4 三元表达式

```cpp
condition ? true_expression : false_expression
```

示例：

```cpp
var max = x > y ? x : y;
```

#### 2.4.5 成员访问表达式

- 对象成员访问：`obj.member`, `obj.method()`
- 指针成员访问：`ptr->member`, `ptr->method()`
- 下标访问：`arr[0]`, `map[key]`

#### 2.4.6 函数调用表达式

```cpp
function_name(arguments)
```

示例：

```cpp
add(10, 20);
print("hello");
```

#### 2.4.7 类型转换表达式

```cpp
expression as type
```

示例：

```cpp
int x = 10;
double y = x as double;
```

#### 2.4.8 类型检查表达式

```cpp
expression is type
```

示例：

```cpp
if (obj is Person) {
  print("obj is a Person");
}
```

#### 2.4.9 范围表达式

```cpp
start .. end
start ..< end
```

示例：

```cpp
var range1 = 1 .. 10;  // 包含 10
var range2 = 1 ..< 10; // 不包含 10

foreach (i in range1) {
  print(i);
}
```

### 2.5 类型

#### 2.5.1 基本类型

- `void`：无类型
- `bool`：布尔类型
- `byte`：无符号字节类型
- `sbyte`：有符号字节类型
- `short`：有符号短整型
- `ushort`：无符号短整型
- `int`：有符号整型
- `uint`：无符号整型
- `long`：有符号长整型
- `ulong`：无符号长整型
- `float`：单精度浮点型
- `double`：双精度浮点型
- `fp16`：半精度浮点型
- `bf16`：Brain Floating Point 16 浮点型
- `char`：字符类型
- `string`：字符串类型
- `string_view`：字符串视图类型

#### 2.5.2 复合类型

##### 2.5.2.1 指针类型

C^ 不掩盖内存的本质，但提供了强大的工具来管理风险。我们保留了指针，但对其进行了**理性约束**。

*   **裸指针**：`Type^`，明确区分于乘法符号 `*`。
*   **空安全**：`Type?^` 表示可空指针，非空指针 `Type^` 永远不为 null。
*   **取地址与解引用**：`^` 取地址（前缀），`^` 解引用（后缀）。

语法：

```cpp
Type^       // 非空指针
Type?^      // 可空指针
```

示例：

```cpp
int value = 42;
int^ ptr = ^value; // 前缀 ^ 取地址
ptr^ = 100;        // 后缀 ^ 解引用

// 可空指针
int?^ safe_ptr = null;

// safe_ptr^ = 5; // 编译错误！必须先检查 null

if (safe_ptr != null) {
  safe_ptr^ = 5; // 编译器流分析：此处安全
}

// 堆内存分配
// new 返回 Type^ (或智能指针，视上下文而定)
Player^ player = new Player(); // 自动调用构造 Player()
delete player; // 自动调用析构 ~
```

##### 2.5.2.2 数组类型

C^ 区分**存储**与**视图**，彻底解决 C 语言数组退化为指针的安全性问题。固定数组是栈上分配的固定大小存储。

*   **固定大小**：在编译时确定大小。
*   **栈数组推导**：`Type[$]`，大小由初始化器推导，栈上分配。
*   **栈上分配**：零开销，无需动态内存管理。
*   **边界检查**：编译期和运行期边界检查，提高安全性。

语法：

```cpp
Type[size]    // 固定大小数组
Type[$]      // 栈数组推导，大小由初始化器自动推导
```

示例：

```cpp
// 栈上固定数组
int[5] numbers = [1, 2, 3, 4, 5];
// numbers[5] = 10; // 编译期错误：越界

// 数组初始化
int[3] empty = [];         // 零初始化
int[4] filled = [1, 2, 3]; // 部分初始化，剩余元素零初始化

// 栈数组推导 - 大小由初始化器自动推导
int[$] arr1 = [1, 2, 3]; // 等价于 int[3]
int[$] arr2 = [1, 2, 3, 4, 5]; // 等价于 int[5]

// 栈数组推导的优势：简洁且零开销
// 编译器在编译期确定大小，分配在栈上
```

##### 2.5.2.3 切片类型

切片是对数组或其他序列的动态视图，包含指针和长度信息，提供边界检查。

*   **动态大小**：在运行时确定大小。
*   **边界检查**：运行期边界检查，提高安全性。
*   **零开销抽象**：切片本身是轻量级的，仅包含指针和长度。
*   **不可变切片**：`Type![]` 表示不可修改的切片，提供额外的安全保障。

语法：

```cpp
Type[]      // 可变切片
Type![]     // 不可变切片
```

**不可变切片特性**：
- `Type![]` 是 `Type[]` 的子类型
- 不能通过下标修改元素：`slice[0] = value` 会产生编译错误
- 可以读取元素：`let x = slice[0]`
- 可以传递给期望 `Type[]` 的函数（协变）

**字符串字面量与不可变切片**：
- 字符串字面量 `"hello"` 的默认类型是 `std.literal_view`
- `std.literal_view` 可以隐式转换为 `byte![]`（不可变字节切片）
- `std.literal_view` 保证指向的数据拥有静态生命周期（.rodata段）

示例：

```cpp
// 切片 (Slice) - 安全的动态视图
func sum_array(int[] arr) -> int {
  int sum = 0;
  // for 循环自动处理切片边界
  for (int x : arr) {
    sum += x;
  }
  return sum;
}

// 传递数组的切片视图
int[5] numbers = [1, 2, 3, 4, 5];
int total = sum_array(numbers[0..3]); // 传递前3个元素

// 切片操作
int[] slice1 = numbers[1..];    // 从索引1到末尾
int[] slice2 = numbers[..3];    // 从开头到索引2
int[] slice3 = numbers[1..3];   // 从索引1到索引2

// 不可变切片
int![] immutable_slice = numbers[0..3]; // 不可变切片
int first = immutable_slice[0]; // 可以读取
// immutable_slice[1] = 20; // 编译错误：不能修改不可变切片

// 不可变切片可以传递给可变切片参数（协变）
func print_ints(int[] arr) { ... }
print_ints(immutable_slice); // OK：int![] 可隐式转换为 int[]

// 字符串字面量默认为 std.literal_view
var s = "hello"; // 类型为 std.literal_view
// s[0] = 'H'; // 编译错误：不能修改字符串字面量

// 字符串字面量可以传递给不可变切片
byte![] bytes = "hello"; // std.literal_view -> byte![]

// 强制栈分配（可变）
byte[$] s_stack = "hello"; // 在栈上分配，可修改
s_stack[0] = 'H'; // OK
```

##### 2.5.2.4 函数类型

```cpp
func(int, int) -> int  // 接受两个整型参数，返回整型的函数类型
func(string) -> void   // 接受字符串参数，返回 void 的函数类型
```

#### 2.5.3 泛型类型

C^ 提供强大的泛型系统，对标 C++ Template，为每个类型生成特定机器码，零运行时开销。

*   **编译期展开**：对标 C++ Template，为每个类型生成特定机器码，零运行时开销。
*   **约束**：使用 `where` 子句或 `requires` 表达式，支持编译期类型约束。
*   **零开销**：泛型实例化在编译期完成，运行时无额外开销。
*   **结构化类型系统**：无需显式声明实现，只要类型满足约束的行为要求即可。

语法：

```cpp
// 泛型类
class ClassName<T> {
  T member;
  
  func method(self, T value) -> T {
    return value;
  }
}

// 带约束的泛型类
class ClassName<T> where Constraint<T> {
  T member;
  
  func method(self&, T value) -> T {
    return value;
  }
}

// 泛型函数
func <T> function_name(T parameter) -> T {
  return parameter;
}

// 带约束的泛型函数
func <T> function_name(T parameter) -> T where Constraint<T> {
  return parameter;
}
```

示例：

```cpp
public class Box<T> {
  T value;
  public func get(self!) -> T => self.value;
}

// 带约束的泛型函数
func max<T>(a: T, b: T) -> T where Comparable<T> {
  return a > b ? a : b;
}

// 使用泛型
Box<int> int_box;
Box<string> string_box;

int max_int = max(10, 20);
string max_string = max("apple", "banana");
```

#### 2.5.4 类型别名

```cpp
using IntList = List<int>;
using StringMap = Map<string, string>;
```

### 2.6 模块系统

#### 2.6.1 模块声明

每个 `.ch` 文件的第一行非注释代码必须是模块声明。模块声明隐式地定义了同名的命名空间。

*   **规则**：模块名是逻辑标识符，通常与目录结构对应。
*   **隐式命名空间**：`module utils.math;` 意味着文件内容默认位于 `utils::math` 命名空间下。

语法：

```cpp
module module_name;
```

示例：

```cpp
// src/utils/math.ch
module utils.math;

public func add(int a, int b) -> int => a + b;
```

#### 2.6.2 导入模块

C^ 采用现代化的模块系统，摒弃了 C/C++ 的头文件包含机制 (`#include`)。

*   **全量导入**：导入整个模块
*   **别名导入**：为模块指定别名，简化使用
*   **C 头文件导入**：直接导入 C 头文件

语法：

```cpp
// 全量导入
import module_path;

// 别名导入
import module_path as alias;

// C 头文件导入
import c "header_file.h";
```

示例：

```cpp
// 导入标准库模块
import std.io;
import std.math;

// 导入用户模块
import utils.math;

// 别名导入
import std.network.http as http;

// 导入 C 头文件
import c "stdio.h";
```

#### 2.6.3 可见性与导出

C^ 采用基于 **访问修饰符 (`public`)** 的隐式导出机制，而非显式的 `export` 语句。

*   **默认 (Internal)**：如果不加修饰符，顶层符号（类、函数、变量）默认为 `internal`，即仅在**当前模块/程序集**内可见。
*   **公开 (Public)**：使用 `public` 修饰的顶层符号，会自动被**导出**，对导入该模块的外部代码可见。
*   **私有 (Private)**：顶层符号可以使用 `private`，表示仅在**当前文件**内可见（文件作用域）。

示例：

```cpp
// src/math.ch
module utils.math;

// 导出符号 (Public)
// 外部代码 import utils.math 后可见
public func add(int a, int b) -> int => a + b;

// 内部符号 (Internal - 默认)
// 仅 utils.math 模块内部可见，外部不可见
func helper() { ... }

// 私有符号 (Private)
// 仅当前文件可见
private const int MAGIC = 42;
```

### 2.7 静态反射与元编程

C^ 提供强大的编译期反射能力，完全零运行时开销，特别适合序列化、ORM 和高性能通用代码。

*   **`@typeof(T)`**：获取类型的元数据对象。
*   **`comptime`**：编译期常量和控制流标记（取代 constexpr）。
*   **`[Attribute]`**：元数据注解。

语法：

```cpp
// 类型元数据
@typeof(T)

// 编译期控制流
comptime {
  // 编译期执行的代码
}

// 编译期循环
comptime for (var f : @typeof(T).fields) {
  // 编译期展开的循环
}

// 属性注解
[AttributeName(argument)]
Type member_name;
```

示例：

```cpp
struct User {
  [JsonKey("user_id")]
  int id;
  string name;
}

// 通用 JSON 序列化函数
func to_json<T>(T! obj) -> string {
  string json = "{";
  bool first = true;
  
  // 编译期展开循环：对 T 的每个字段生成代码
  comptime for (var f : @typeof(T).fields) {
    if (!first) json += ", ";
    first = false;
    
    json += "\"" + f.name + "\": ";
    
    // 编译期多态：根据字段类型选择合适的转换
    if (f.type == string) {
      json += "\"" + obj.@field(f) + "\"";
    } else {
      json += to_string(obj.@field(f));
    }
  }
  
  json += "}";
  return json;
}

// 使用
User u = {1, "Alice"};
print(to_json(u)); 
```



## 3. 语义规则

### 3.1 作用域规则

- **全局作用域**：在所有函数和类之外声明的变量和函数
- **函数作用域**：在函数内部声明的变量和函数
- **类作用域**：在类内部声明的成员变量和成员函数
- **块作用域**：在复合语句内部声明的变量

### 3.2 类型规则

- **类型推断**：如果变量没有显式标注类型，编译器会根据初始化表达式推断类型
- **类型兼容性**：某些类型之间可以隐式转换，如 `int` 到 `double`
- **类型检查**：编译器会检查表达式和语句的类型是否正确

### 3.3 内存管理

- **手动内存管理**：C^ 语言使用 `new` 和 `delete` 进行手动内存管理
- **内存安全**：通过类型系统和编译器检查提供内存安全保障
- **智能指针**：标准库提供智能指针，支持自动内存管理

### 3.4 异常处理

- **异常抛出**：使用 `throw` 关键字抛出异常
- **异常捕获**：使用 `try-catch` 语句捕获异常
- **异常类型**：异常可以是任何类型，但通常是 `Exception` 类的子类

## 4. 代码风格

### 4.1 命名约定

- **变量和函数**：使用小驼峰命名法（camelCase）
- **类和结构体**：使用大驼峰命名法（PascalCase）
- **常量**：使用全大写字母，下划线分隔（SNAKE_CASE）
- **命名空间**：使用大驼峰命名法（PascalCase）

### 4.2 缩进和空格

- 使用 4 个空格进行缩进
- 在运算符周围添加空格
- 在逗号和分号后添加空格
- 在大括号前后添加空格

### 4.3 注释

- **单行注释**：使用 `//`
- **多行注释**：使用 `/* */`
- **文档注释**：使用 `///` 或 `/** */`

## 5. 示例代码

### 5.1 基本示例

```cpp
import std.io;

func main() -> int {
  int x = 10;
  int y = 20;
  int sum = x + y;
  
  print("Sum: " + sum);
  
  if (sum > 25) {
    print("Sum is greater than 25");
  } else if (sum == 25) {
    print("Sum is exactly 25");
  } else {
    print("Sum is less than 25");
  }
  
  for (int i = 0; i < 5; i++) {
    print("Iteration " + i);
  }
  
  return 0;
}
```

### 5.2 类示例

```cpp
import std.io;

class Person {
  string name;
  int age;
  
  func init(string name, int age) {
    self.name = name;
    self.age = age;
  }
  
  func get_name() -> string {
    return self.name;
  }
  
  func get_age() -> int {
    return self.age;
  }
  
  func set_age(int new_age) {
    if (new_age > 0) {
      self.age = new_age;
    }
  }
  
  func to_string() -> string {
    return "Person{name: " + self.name + ", age: " + self.age + "}";
  }
}

class Student : Person {
  string student_id;
  
  func init(string name, int age, string student_id) {
    base.init(name, age);
    self.student_id = student_id;
  }
  
  func get_student_id() -> string {
    return self.student_id;
  }
  
  func to_string() -> string {
    return "Student{name: " + self.name + ", age: " + self.age + ", student_id: " + self.student_id + "}";
  }
}

func main() -> int {
  var person = Person("John", 30);
  print(person.to_string());
  
  var student = Student("Alice", 20, "S12345");
  print(student.to_string());
  
  return 0;
}
```

### 5.3 泛型示例

```cpp
import std.io;

func print_all<T>(T[] values) {
  foreach (value in values) {
    print(value);
  }
}

func max<T>(T a, T b) -> T where Comparable<T> {
  return a > b ? a : b;
}

func *generate_values<T>(T[] values) -> T {
  foreach (value in values) {
    yield value;
  }
}

func main() -> int {
  int[] int_values = [1, 2, 3, 4, 5];
  string[] string_values = ["hello", "world", "c-hat"];
  
  print("Integer values:");
  print_all(int_values);
  
  print("String values:");
  print_all(string_values);
  
  int a = 10;
  int b = 20;
  print("Max of " + a + " and " + b + " is " + max(a, b));
  
  var generator = generate_values(int_values);
  print("Generated values:");
  foreach (value in generator) {
    print(value);
  }
  
  return 0;
}
```

### 5.4 异步示例

```cpp
import std.io;
import std.net;

func fetch_url(string url) -> string {
  var client = net.HttpClient();
  var response = await client.get(url);
  return response.body;
}

func main() -> int {
  print("Fetching data...");
  var data = await fetch_url("https://example.com");
  print("Fetched data length: " + data.length);
  return 0;
}
```

## 6. 总结

本文档详细介绍了 C^ 语言的语法规范，包括词法元素、语法结构、语句、表达式、语义规则等内容。这些规范为 C^ 语言编译器的开发提供了清晰的参考，有助于确保编译器能够正确解析和处理 C^ 语言代码。

通过遵循这些语法规范，开发团队可以更加高效地实现 C^ 语言编译器，同时也为 C^ 语言的使用者提供了清晰的语言参考。
