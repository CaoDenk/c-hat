# C^ 语言运算符优先级规范

本文档详细定义 C^ 语言所有运算符的优先级和结合性，为语法分析器提供精确规范。

## 1. 设计原则

1.  **后缀优先 (Suffix First)**：为了支持流畅的链式调用，所有后缀运算符（包括成员访问 `.`、调用 `()`、后缀解引用 `^`、移动 `~`）必须拥有最高优先级。
2.  **数学直觉 (Mathematical Intuition)**：幂运算 `**` 高于乘除，且为右结合。
3.  **理性修正 (Rational Fixes)**：修复 C/C++ 中历史上反直觉的位运算优先级问题（如 `&` 低于 `==` 的陷阱）。

## 2. 优先级表（从高到低）

优先级从高到低排列。同一行中的运算符优先级相同。

| 优先级 | 运算符                 | 说明                                           | 结合性     |
| :----- | :--------------------- | :--------------------------------------------- | :--------- |
| **1**  | `()` `[]` `.` `->`     | 函数调用、数组索引、成员访问、**指针成员访问** | 左到右     |
|        | `^` `~` `++` `--`      | **后缀操作符**：解引用、移动语义、后缀自增/减  | 左到右     |
|        | `!` (后缀)             | 显式非空断言 (可选特性)                        | 左到右     |
| **2**  | `**`                   | **幂运算** (标准库实现)                        | **右到左** |
| **3**  | `!` `+` `-` `++` `--`  | **前缀操作符**：逻辑非、正负号、前缀自增/减    | 右到左     |
|        | `~`                    | 按位取反 (前缀)                                | 右到左     |
|        | `^`                    | **取地址** (前缀)                              | 右到左     |
|        | `$`                    | **倒序索引** (前缀，如 `$1` 表示倒数第一)      | 右到左     |
|        | `sizeof` `typeof`      | 编译期操作符                                   | 右到左     |
|        | `new` `delete` `await` | 内存与协程操作                                 | 右到左     |
| **4**  | `*` `/` `%`            | 乘、除、取模                                   | 左到右     |
| **5**  | `+` `-`                | 加、减                                         | 左到右     |
| **6**  | `<<` `>>`              | 位移                                           | 左到右     |
| **7**  | `&`                    | **按位与** (注：优先级高于比较)                | 左到右     |
| **8**  | `^`                    | **按位异或**                                   | 左到右     |
| **9**  | `|`                    | **按位或**                                     | 左到右     |
| **10** | `<` `<=` `>` `>=`      | 关系比较                                       | 左到右     |
|        | `is` `as`              | 类型检查与转换                                 | 左到右     |
| **11** | `==` `!=`              | 相等性比较                                     | 左到右     |
| **12** | `&&`                   | 逻辑与                                         | 左到右     |
| **13** | `||`                   | 逻辑或                                         | 左到右     |
| **14** | `..`                   | 范围操作符 (Range)                             | 左到右     |
| **15** | `?:`                   | 三元条件                                       | **右到左** |
| **16** | `=` `+=` `-=` `*=` ... | 赋值与复合赋值                                 | **右到左** |
| **17** | `=>`                   | Lambda/Match 箭头                              | 左到右     |
| **18** | `,`                    | 逗号操作符                                     | 左到右     |

---

## 3. 关键设计解析

### 3.1 后缀 `^` 与成员访问

为了支持 C^ 特有的指针解引用语法，后缀 `^` 必须具有最高优先级（Level 1）。

*   **场景**：访问指针指向对象的成员。
*   **语法**：`ptr^.field`
*   **解析**：`(ptr^).field`
*   **对比 C++**：C^ 保留了 `->` 作为指针成员访问的语法糖，同时也支持正交的 `^.` 写法。

```cpp
struct Point { int x; int y; }
Point^ p = new Point(10, 20);

// C^ 推荐方式 (C++ 兼容)：
int val1 = p->x;

// 正交方式 (底层等价)：
int val2 = p^.x; 
// 解析顺序：(p^).x
```

### 3.2 幂运算符 `**`

C^ 引入 `**` 作为幂运算符，填补了 C 家族语言的空白。为了符合数学直觉，**其优先级高于前缀负号**。

*   **优先级**：`**` (Level 2) > `-` (Level 3)。
*   **右结合性**：`2 ** 3 ** 2` 等价于 `2 ** (3 ** 2)` = `2 ** 9` = 512。
*   **负号交互**：
    *   `-2 ** 2` 解析为 `-(2 ** 2) = -4` （符合 Python/Ruby 习惯，而非 Excel）。
    *   `2 ** -2` 解析为 `2 ** (-2) = 0.25` （右结合性自然吞噬了右侧的一元表达式）。

*   **实现**：语言本身不内置 `**` 的汇编实现，而是依赖标准库重载。
*   **示例**：
    ```cpp
    // 标准库 Math 模块
    func operator**(double base, double exp) -> double {
        return pow(base, exp);
    }
    
    // 使用
    double res = 2.0 ** 3.0 + 1.0; // 解析：(2.0 ** 3.0) + 1.0
    ```

### 3.3 位运算优先级修正 (Rational Bitwise Precedence)

C/C++ 的一个历史包袱是位运算 (`&` `|` `^`) 的优先级低于比较运算 (`==` `!=`)。
这导致 `if (flags & MASK == 0)` 被错误解析为 `if (flags & (MASK == 0))`。

**C^ 修正方案**：
将位运算优先级提升至关系比较 (`<` `>`) 之前。

*   **新行为**：`flags & MASK == 0` 解析为 `(flags & MASK) == 0`。
*   **理由**：符合直觉，减少括号噪音，消除常见 Bug。

## 4. 运算符详细说明

### 4.1 后缀运算符（优先级1，左结合）

```cpp
// 函数调用
func add(int a, int b) -> int { return a + b; }
var result = add(1, 2);  // () 函数调用

// 数组索引
int[] arr = [1, 2, 3, 4, 5];
var first = arr[0];  // [] 数组索引

// 成员访问
class Point { int x; int y; }
Point p;
var x = p.x;  // . 成员访问

// 指针成员访问
Point^ ptr = new Point(10, 20);
var val = ptr->x;  // -> 指针成员访问

// 后缀解引用
var val2 = ptr^.x;  // 等价于 (ptr^).x

// 后缀自增/自减
var i = 5;
var j = i++;  // j = 5, i = 6
var k = i--;  // k = 6, i = 5

// 移动语义
var original = Resource();
var moved = original~;  // original 所有权转移到 moved
```

### 4.2 前缀运算符（优先级3，右结合）

```cpp
// 取地址
int x = 5;
int^ ptr = ^x;  // ^ 取地址

// 前缀自增/自减
var i = 5;
var j = ++i;  // j = 6, i = 6
var k = --i;  // k = 5, i = 5

// 一元正负
var positive = +5;
var negative = -5;

// 逻辑非
var flag = true;
var notFlag = !flag;  // false

// 按位取反
var mask = 0xFF;
var inverted = ~mask;  // 0xFFFFFF00（32位）

// 倒序索引
int[] arr = [1, 2, 3, 4, 5];
var last = arr[$1];  // 5 (倒数第一个)

// sizeof 和 typeof
var size = sizeof(int);
var type = typeof(x);

// new 和 delete
int^ p = new int(42);
delete p;
```

### 4.3 算术运算符（优先级4-5，左结合）

```cpp
// 乘法、除法、取模（优先级4）
var product = 3 * 4;   // 12
var quotient = 10 / 3; // 3（整数除法）
var remainder = 10 % 3; // 1

// 加法和减法（优先级5）
var sum = 3 + 4;       // 7
var diff = 10 - 3;     // 7
```

### 4.4 移位运算符（优先级6，左结合）

```cpp
// 左移
var shiftedLeft = 1 << 3;  // 8 (0b1000)

// 右移（算术右移，保留符号位）
var shiftedRight = -8 >> 2;  // -2
```

### 4.5 位运算符（优先级7-9，左结合）

```cpp
// 按位与（优先级7）
var andResult = 0x0F & 0xF0;  // 0x00

// 按位异或（优先级8）
var xorResult = 0x0F ^ 0xF0;  // 0xFF

// 按位或（优先级9）
var orResult = 0x0F | 0xF0;   // 0xFF
```

### 4.6 关系运算符（优先级10，左结合）

```cpp
// 小于、小于等于
var less = 3 < 5;       // true
var lessOrEqual = 5 <= 5;  // true

// 大于、大于等于
var greater = 5 > 3;    // true
var greaterOrEqual = 5 >= 5;  // true

// 类型检查和转换
var isInt = x is int;
var asString = x as string;
```

### 4.7 相等运算符（优先级11，左结合）

```cpp
// 相等
var equal = 5 == 5;     // true

// 不等
var notEqual = 5 != 3;  // true

// 注意：对于可空类型，== 和 != 需要处理 null
int?^ p1 = null;
int?^ p2 = null;
var same = p1 == p2;  // true（都是null）
```

### 4.8 逻辑运算符（优先级12-13，左结合）

```cpp
// 逻辑与（优先级12，短路求值）
var bothTrue = true && false;  // false
// 如果左侧为false，右侧不会执行

// 逻辑或（优先级13，短路求值）
var eitherTrue = true || false;  // true
// 如果左侧为true，右侧不会执行
```

### 4.9 范围运算符（优先级14，左结合）

```cpp
// 范围表达式
int[] numbers = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];

// 数组切片
int[] slice1 = numbers[0..5]; // [0, 1, 2, 3, 4]
int[] slice2 = numbers[2..8]; // [2, 3, 4, 5, 6, 7]

// 范围用于循环
for (int i in 0..9) {
    println(i);
}
```

### 4.10 条件运算符（优先级15，右结合）

```cpp
// 三元条件表达式
var abs = x >= 0 ? x : -x;

// 右结合：可以嵌套
var sign = x > 0 ? 1 : (x < 0 ? -1 : 0);
// 等价于：x > 0 ? 1 : x < 0 ? -1 : 0
```

### 4.11 赋值运算符（优先级16，右结合）

```cpp
// 简单赋值
var x = 5;

// 复合赋值
x += 3;   // x = x + 3
x -= 2;   // x = x - 2
x *= 4;   // x = x * 4
x /= 2;   // x = x / 2
x %= 3;   // x = x % 3

// 位复合赋值
x &= 0xFF;
x |= 0x10;
x ^= 0xFF;
x <<= 2;
x >>= 1;

// 右结合：支持链式赋值
var a, b, c;
a = b = c = 0;  // 全部赋值为0
```

### 4.12 Lambda箭头（优先级17，左结合）

```cpp
// Lambda表达式
var add = [](int a, int b) => a + b;

// 箭头函数（简写）
var multiply = (a, b) => a * b;

// 匹配表达式
var result = match (x) {
    1 => "one",
    2 => "two",
    _ => "other"
};
```

### 4.13 逗号运算符（优先级18，左结合）

```cpp
// 顺序求值
var a = (1, 2, 3);  // a = 3（最后一个表达式的值）

// 在for循环中使用
for (var i = 0, j = 10; i < j; i++, j--) {
    // ...
}
```

## 5. 优先级和结合性示例

### 5.1 复杂表达式解析

```cpp
// 示例1：算术和比较
var result = 3 + 4 * 5 > 10 && 2 < 5;
// 解析步骤：
// 1. 4 * 5 = 20
// 2. 3 + 20 = 23
// 3. 23 > 10 = true
// 4. 2 < 5 = true
// 5. true && true = true

// 示例2：赋值和算术
var a = 5;
var b = 3;
var c = a + b * 2;
// 解析：b * 2 = 6, a + 6 = 11, c = 11

// 示例3：位运算和比较（C^ 修正后的行为）
var flags = 0x0F;
var mask = 0x03;
var check = flags & mask == 0x03;
// C^ 解析：(flags & mask) == 0x03 = true
// C++ 解析：flags & (mask == 0x03) = 0x0F & true = 1 (错误!)

// 示例4：逻辑运算符短路
var x = 0;
if (x != 0 && 10 / x > 5) {  // 安全，不会除以0
    // ...
}

// 示例5：条件表达式嵌套
var category = age < 13 ? "child" :
               age < 20 ? "teenager" :
               age < 65 ? "adult" : "senior";

// 示例6：幂运算
var power = 2 ** 3 ** 2;
// 解析：2 ** (3 ** 2) = 2 ** 9 = 512

// 示例7：负号与幂运算
var negPower = -2 ** 2;
// 解析：-(2 ** 2) = -4
```

### 5.2 使用括号明确优先级

```cpp
// 不明确（容易出错）
var result = a & b == c;

// 明确（推荐）
var result = (a & b) == c;

// 复杂表达式
var value = (a + b) * (c - d) / (e + f);
```

## 6. 运算符重载

### 6.1 可重载运算符

| 运算符 | 函数名 | 示例 |
|--------|--------|------|
| `+` | `operator+` | `a + b` |
| `-` | `operator-` | `a - b` |
| `*` | `operator*` | `a * b` |
| `/` | `operator/` | `a / b` |
| `%` | `operator%` | `a % b` |
| `**` | `operator**` | `a ** b` |
| `==` | `operator==` | `a == b` |
| `!=` | `operator!=` | `a != b` |
| `<` | `operator<` | `a < b` |
| `<=` | `operator<=` | `a <= b` |
| `>` | `operator>` | `a > b` |
| `>=` | `operator>=` | `a >= b` |
| `&` | `operator&` | `a & b` |
| `|` | `operator|` | `a | b` |
| `^` | `operator^` | `a ^ b` |
| `<<` | `operator<<` | `a << b` |
| `>>` | `operator>>` | `a >> b` |
| `[]` | `operator[]` | `a[b]` |
| `()` | `operator()` | `a(b)` |

### 6.2 运算符重载示例

```cpp
class Vector2 {
    float x, y;
    
    // 加法
    func operator+(self!, Vector2 other) -> Vector2 {
        return Vector2(self.x + other.x, self.y + other.y);
    }
    
    // 相等比较
    func operator==(self!, Vector2 other) -> bool {
        return self.x == other.x && self.y == other.y;
    }
    
    // 数组索引
    func operator[](self!, int index) -> float {
        if (index == 0) return self.x;
        if (index == 1) return self.y;
        throw IndexError("Index out of range");
    }
}
```

## 7. 常见错误

### 7.1 优先级错误

```cpp
// 错误：意图检查第2位是否为1
if (flags & 1 << 2 == 0) { ... }
// 实际：flags & (1 << 2 == 0)
// 应该：(flags & (1 << 2)) == 0

// 错误：赋值优先级低于比较
if (x = 5 == 5) { ... }
// 实际：x = (5 == 5)
// 应该：x = 5; if (x == 5) { ... }
```

### 7.2 结合性错误

```cpp
// 错误：赋值右结合
a = b = c = d;  // 正确：a = (b = (c = d))

// 错误：条件表达式右结合
a ? b : c ? d : e;  // 正确：a ? b : (c ? d : e)
```

## 8. 与C/C++的区别

| 特性 | C/C++ | C^ |
|------|-------|-----|
| 指针解引用 | `*ptr` | `ptr^` |
| 取地址 | `&var` | `^var` |
| 幂运算 | 无 | `**` |
| 位运算优先级 | 低于比较 | 高于比较 |
| 范围 | 无 | `..` |
| 倒序索引 | 无 | `$` |

## 9. 实现注意事项

### 9.1 语法分析器实现

```cpp
// 使用优先级爬升算法（Pratt Parser）
// 或递归下降配合优先级表

Expression* parseExpression(int minPrecedence = 0) {
    auto left = parsePrimary();
    
    while (currentPrecedence() >= minPrecedence) {
        auto op = currentOperator();
        auto precedence = getPrecedence(op);
        auto associativity = getAssociativity(op);
        
        advance();
        
        int nextMinPrecedence = precedence;
        if (associativity == LEFT) {
            nextMinPrecedence++;
        }
        
        auto right = parseExpression(nextMinPrecedence);
        left = new BinaryExpr(op, left, right);
    }
    
    return left;
}
```

### 9.2 错误恢复

```cpp
// 当遇到意外的运算符时
if (!isValidInfixOperator(current())) {
    error("Unexpected operator");
    // 跳过并继续
    advance();
    return parseExpression(minPrecedence);
}
```

## 10. 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0 | 2025-02 | 初始版本，定义完整优先级表 |
| 1.1 | 2025-02 | 添加幂运算符 `**`，修正位运算优先级 |
