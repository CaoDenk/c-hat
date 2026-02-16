# C^ 语言语句规范

本文档详细定义 C^ 语言的所有语句类型，包括语法、语义和执行规则。

## 1. 语句概述

C^ 语言中的语句分为以下几类：

- **声明语句**：变量声明、函数声明、类型声明
- **表达式语句**：表达式后跟分号
- **复合语句**：用 `{}` 包围的语句块
- **控制流语句**：条件、循环、跳转
- **异常处理语句**：try-catch-throw
- **资源管理语句**：defer

## 2. 声明语句

### 2.1 变量声明

#### 语法

```
variable_declaration ::= ('var' | 'let' | 'const') identifier (':' type)? ('=' expression)? ';'
```

#### 语义

- `var`：可变变量，可以重新赋值
- `let`：不可变变量，初始化后不能修改
- `const`：编译期常量，必须在声明时初始化

#### 示例

```cpp
// var - 可变变量
var x = 5;
x = 10;  // OK

// let - 不可变变量
let y = 5;
// y = 10;  // 错误：不能修改 let 变量

// const - 编译期常量
const int MAX_SIZE = 100;
// MAX_SIZE = 200;  // 错误：不能修改 const

// 显式类型声明
int count = 0;
string name = "C^";

// 类型推导
var inferred = 3.14;  // double 类型
```

### 2.2 延迟初始化声明

#### 语法

```
late_declaration ::= 'late' type identifier ';'
late_initialization ::= identifier '=' expression ';'
```

#### 语义

- `late` 声明的变量在声明时不构造
- 在使用前必须初始化
- 编译器会检查所有代码路径是否都初始化了变量

#### 示例

```cpp
class FileReader {
    late File file;
    
    func open(string path) {
        file = File(path);  // 延迟初始化
    }
    
    func read() -> string {
        // 编译器确保 file 已初始化
        return file.readAll();
    }
}
```

## 3. 表达式语句

### 3.1 语法

```
expression_statement ::= expression ';'
```

### 3.2 示例

```cpp
// 函数调用表达式语句
print("Hello");

// 赋值表达式语句
x = 5;

// 自增/自减表达式语句
i++;
--j;

// 复合赋值
a += b;

// new/delete 表达式
var p = new int(5);
delete p;
```

## 4. 复合语句（代码块）

### 4.1 语法

```
compound_statement ::= '{' statement* '}'
```

### 4.2 语义

- 创建新的作用域
- 块内声明的变量在块外不可见
- 支持嵌套

### 4.3 示例

```cpp
{
    var x = 5;
    {
        var y = 10;
        var x = 20;  // 遮蔽外层的 x
        print(x);    // 20
    }
    print(x);  // 5
    // print(y);  // 错误：y 不可见
}
```

## 5. 条件语句

### 5.1 if 语句

#### 语法

```
if_statement ::= 'if' '(' expression ')' statement ('else' statement)?
```

#### 语义

- **严格布尔类型**：C^ 采用严格的布尔检查
- **禁止隐式转换**：数值类型和指针类型**不能**直接作为条件，必须显式比较
- `else` 分支可选
- 使用悬挂 else 规则：else 匹配最近的未匹配的 if

#### 示例

```cpp
// 布尔表达式
if (x > 0) {
    print("Positive");
} else if (x < 0) {
    print("Negative");
} else {
    print("Zero");
}

// 数值类型 (强制显式比较)
// if (x) { } // Error
if (x != 0) {
    print("x is not zero");
}

// 指针类型 (强制显式比较)
// if (ptr) { } // Error
if (ptr != null) {
    print("ptr is not null");
}

// 单语句可以省略大括号（但不推荐）
if (x == 0) print("Zero");
```

### 5.2 match 语句

match 是一个表达式（Expression），具有返回值。C^ 移除了传统的 `switch` 语句，采用更强大、更安全的模式匹配。

#### 语法

```
match_statement ::= 'match' '(' expression ')' '{' match_arm* '}'
match_arm ::= pattern ('if' expression)? '=>' (expression | statement) ','?
pattern ::= literal | identifier | '_' | pattern '|' pattern | pattern '(' pattern_list ')'
```

#### 语义

- 必须覆盖所有可能的情况（穷尽性检查）
- `_` 表示默认匹配（必选，除非所有情况都被覆盖）
- 支持模式或（`|`）
- 支持守卫子句 (`if condition`)
- 支持结构解构 (Destructuring)
- 支持字符串字面量匹配

#### 示例

```cpp
// 基本用法
match (statusCode) {
    200 => print("OK"),
    404 => print("Not Found"),
    500 => print("Server Error"),
    _ => print("Unknown")
}

// 作为表达式
var message = match (statusCode) {
    200 => "OK",
    404 => "Not Found",
    500 => "Server Error",
    _ => "Unknown"
};

// 多模式匹配
match (value) {
    1 | 2 | 3 => print("Small"),
    4 | 5 | 6 => print("Medium"),
    _ => print("Large")
}

// 守卫子句 (Guards)
match (num) {
    case x if x % 2 == 0 => "Even",
    case _ => "Odd"
}

// 结构解构 (Destructuring)
match (point) {
    case Point(0, 0) => "Origin",
    case Point(x, 0) => "On X-axis at " + x,
    case Point(0, y) => "On Y-axis at " + y,
    case Point(x, y) => $"({x}, {y})"
}

// 字符串匹配 (编译器优化为哈希跳转)
match (command) {
    case "start" => start_game(),
    case "quit" => exit_game(),
    case _ => print("Unknown command")
}
```

#### 穷尽性检查

match 必须覆盖所有可能的情况。对于枚举类型，如果未覆盖所有成员且没有 `_` 分支，编译器会报错：

```cpp
enum Color { Red, Green, Blue }

// 编译错误：未覆盖 Blue
// match (color) {
//     Red => ...
//     Green => ...
// }
```

### 5.3 三元运算符

#### 语法

```
ternary_expression ::= expression '?' expression ':' expression
```

#### 语义

- 条件表达式求值后，如果是 true，则求值并返回中间表达式，否则求值并返回后面的表达式
- 整个三元运算符是一个表达式，可以作为其他表达式的一部分

#### 示例

```cpp
// 基本用法
int max = (x > y) ? x : y;

// 嵌套使用
int result = x > 0 ? 1 : (x < 0 ? -1 : 0);

// 作为表达式的一部分
printf("Result: %d\n", condition ? value1 : value2);
```

#### 注意事项

- 可读性：避免过度嵌套三元运算符
- 类型一致性：true_expression 和 false_expression 的类型应保持一致
- 副作用：表达式中的副作用会根据条件执行

## 6. 循环语句

### 6.1 while 循环

#### 语法

```
while_statement ::= 'while' '(' expression ')' statement
```

#### 语义

- 先检查条件，后执行循环体
- 条件必须是 `bool` 类型

#### 示例

```cpp
var i = 0;
while (i < 10) {
    print(i);
    i++;
}
```

### 6.2 do-while 循环

#### 语法

```
do_while_statement ::= 'do' statement 'while' '(' expression ')' ';'
```

#### 语义

- 先执行循环体，后检查条件
- 循环体至少执行一次

#### 示例

```cpp
var input;
do {
    input = readInput();
    process(input);
} while (input != "quit");
```

### 6.3 for 循环

#### 语法

```
for_statement ::= 'for' '(' for_init? ';' expression? ';' for_update? ')' statement
for_init ::= variable_declaration | expression_list
for_update ::= expression_list
expression_list ::= expression (',' expression)*
```

#### 语义

- `for_init`：初始化，可以是变量声明或表达式列表
- 条件表达式：可选，默认为 `true`
- `for_update`：每次迭代后执行

#### 示例

```cpp
// 基本 for 循环
for (var i = 0; i < 10; i++) {
    print(i);
}

// 多变量
for (var i = 0, j = 10; i < j; i++, j--) {
    print(i, j);
}

// 无限循环
for (;;) {
    // 无限循环
}

// 省略部分
var i = 0;
for (; i < 10;) {
    i++;
}
```

### 6.4 范围 for 循环（预留）

#### 语法

```
range_for_statement ::= 'for' '(' ('var' | 'let') identifier ':' expression ')' statement
```

#### 示例

```cpp
// 遍历数组
for (var item : array) {
    print(item);
}

// 使用 let 表示不可变
for (let char : string) {
    // char 是只读的
}

// 遍历范围
for (var i : 0..10) {
    print(i);
}
```

## 7. 跳转语句

### 7.1 break 语句

#### 语法

```
break_statement ::= 'break' ';'
```

#### 语义

- 立即终止最内层的循环或 switch
- 只能出现在循环或 switch 内部

#### 示例

```cpp
for (var i = 0; i < 100; i++) {
    if (i == 50) {
        break;  // 退出循环
    }
    print(i);
}
```

### 7.2 continue 语句

#### 语法

```
continue_statement ::= 'continue' ';'
```

#### 语义

- 跳过当前迭代的剩余部分，开始下一次迭代
- 只能出现在循环内部

#### 示例

```cpp
for (var i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;  // 跳过偶数
    }
    print(i);  // 只打印奇数
}
```

### 7.3 return 语句

#### 语法

```
return_statement ::= 'return' expression? ';'
```

#### 语义

- 从函数返回
- 如果函数返回类型不是 `void`，必须提供返回值
- 如果函数返回类型是 `void`，可以省略返回值

#### 示例

```cpp
func add(int a, int b) -> int {
    return a + b;
}

func printMessage(string msg) -> void {
    print(msg);
    return;  // 可选
}

// 多返回值
func divide(int a, int b) -> (int, int) {
    return (a / b, a % b);
}
```

### 7.4 throw 语句

#### 语法

```
throw_statement ::= 'throw' expression ';'
```

#### 语义

- 抛出异常
- 表达式必须是异常类型

#### 示例

```cpp
func divide(int a, int b) -> int {
    if (b == 0) {
        throw DivisionByZeroError("Cannot divide by zero");
    }
    return a / b;
}
```

## 8. 异常处理语句

### 8.1 try-catch 语句

#### 语法

```
try_statement ::= 'try' statement catch_clause*
catch_clause ::= 'catch' '(' type identifier? ')' statement
```

#### 语义

- `try` 块中的代码被执行
- 如果抛出异常，匹配的 `catch` 块被执行
- 如果没有匹配的 `catch`，异常继续传播

#### 示例

```cpp
try {
    var result = divide(10, 0);
    print(result);
} catch (DivisionByZeroError e) {
    print("Error: " + e.message);
} catch (Error e) {
    print("Unknown error: " + e.message);
}

// 捕获所有异常
try {
    riskyOperation();
} catch (Error e) {
    print("Caught: " + e.message);
}
```

## 9. 资源管理语句

### 9.1 defer 语句

#### 语法

```
defer_statement ::= 'defer' statement
```

#### 语义

- **块级作用域**：`defer` 绑定的语句在包含它的最近一层代码块（`{}`）结束前执行。
- **LIFO 执行顺序**：同一作用域内多个 `defer` 按照后进先出（逆序）执行。
- **变量捕获**：以引用方式捕获当前上下文变量。
- **异常安全**：即使发生异常退出作用域，`defer` 也会执行。

#### 示例

```cpp
func processFile(string path) {
    var file = File.open(path);
    defer file.close();  // 作用域结束时关闭
    
    // 支持代码块
    defer {
        print("Exiting scope");
        log("Done");
    }
    
    if (file.isEmpty()) {
        return;  // file.close() 在这里执行
    }
    
    // ...
} // file.close() 在这里执行

// 嵌套作用域
{
    defer print("Inner exit");
    print("Inside");
} // 打印 "Inner exit"
print("Outside");
```

## 10. 空语句

### 10.1 语法

```
empty_statement ::= ';'
```

### 10.2 语义

- 什么都不做
- 偶尔在循环中使用

### 10.3 示例

```cpp
// 空循环体
while (processNext()) {
    ;  // 空语句
}
```

## 11. 标签语句（预留）

### 11.1 语法

```
labeled_statement ::= identifier ':' statement
goto_statement ::= 'goto' identifier ';'
```

### 11.2 示例

```cpp
// 不推荐，但支持
func example() {
    var i = 0;
    start:
    print(i);
    i++;
    if (i < 10) {
        goto start;
    }
}
```

## 12. 语句执行规则

### 12.1 正常执行

语句按顺序执行，除非遇到控制流语句。

### 12.2 异常传播

如果语句抛出异常：
1. 当前语句停止执行
2. 在当前作用域查找匹配的 catch
3. 如果没有找到，异常传播到外层作用域
4. 传播前执行所有 defer 语句

### 12.3 作用域规则

- 每个复合语句创建新作用域
- 变量在声明时创建，在作用域结束时销毁
- 内部作用域可以遮蔽外部作用域的变量

## 13. 与 C/C++ 的区别

| 特性     | C/C++  | C^                 |
| -------- | ------ | ------------------ |
| switch   | 支持   | 使用 match         |
| case     | 支持   | 使用 =>            |
| default  | 支持   | 使用 _             |
| defer    | 无     | 支持               |
| 范围 for | C++11+ | 支持               |
| 变量声明 | 随处   | 随处，支持类型推导 |

## 14. 版本历史

| 版本 | 日期    | 变更     |
| ---- | ------- | -------- |
| 1.0  | 2025-02 | 初始版本 |
