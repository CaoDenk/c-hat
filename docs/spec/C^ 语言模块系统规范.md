# C^ 语言模块系统规范

## 1. 概述

C^ 语言采用现代化的模块系统，摒弃了传统 C/C++ 的头文件包含机制（`#include`）和显式 `namespace` 关键字，提供了更清晰、更安全的代码组织方式。本文档详细描述 C^ 语言中模块的声明、导入、可见性控制等模块系统的各个方面，与设计文档保持一致。

## 2. 模块基础

### 2.1 模块概念

- **模块**：C^ 中的模块是代码组织的基本单位，一个模块由一个或多个 `.ch` 文件组成
- **模块名**：模块名是一个逻辑标识符，通常与目录结构对应
- **模块文件**：每个 `.ch` 文件是模块的一部分，文件顶部需要声明所属的模块
- **物理结构即逻辑结构**：模块声明同时定义编译单元边界和符号作用域

### 2.2 文件扩展名

C^ 建议采用以下文件扩展名：
- **`.ch`**：C^ 源代码文件（包含声明与实现）
- **`.hh`**：自动生成的 C++ 兼容头文件（仅用于导出库给 C++ 使用，C^ 内部不使用）

### 2.3 模块文件结构

```
src/
├── utils/
│   ├── math.ch     # 属于 utils.math 模块
│   └── string.ch   # 属于 utils.string 模块
├── network/
│   ├── http.ch     # 属于 network.http 模块
│   └── tcp.ch      # 属于 network.tcp 模块
└── main.ch         # 属于 main 模块
```

## 3. 模块声明

### 3.1 基本模块声明

每个 `.ch` 文件的第一行非注释代码必须是模块声明：

```cpp
// 模块声明 - 同时隐式定义命名空间
module utils.math;

// 此时符号 add 自动位于 utils.math.add
public func add(int a, int b) -> int => a + b;
```

### 3.2 模块名规则

- 模块名由点分隔的标识符组成，如 `utils.math`、`network.http.client`
- 模块名通常与文件系统目录结构对应
- 模块名应该使用小写字母，单词之间用点分隔
- **文件名不得包含 `.`**：源文件名只能包含字母、数字、下划线，扩展名固定为 `.ch`

### 3.3 目录映射规则

模块路径 `std.io.file` 严格映射到目录结构：
- 查找顺序：
  1. `std/io/file.ch` (文件模块)
  2. `std/io/file/mod.ch` (目录模块入口，类似 Rust `mod.rs`)

### 3.4 嵌套命名空间

由于移除了显式 `namespace` 关键字，C^ 采用以下方式处理嵌套命名空间：
1. **子模块**：创建子目录和新文件（例如 `utils/math/impl.ch` 对应 `module utils.math.impl`）
2. **静态结构体**：使用 `static struct` 作为逻辑容器

```cpp
module utils.math;

// 使用静态结构体模拟嵌套命名空间
public static struct Complex {
    public struct Data { ... }
    public func create() { ... }
}
// 使用: utils.math.Complex.Data
```

### 3.5 多文件模块

C^ 允许将一个逻辑模块拆分到多个物理文件中：
- 多个文件可以声明同一个模块名 `module utils.math;`
- 编译器会将这些文件视为同一个逻辑单元
- **Internal** 符号：在同一个模块的所有文件中可见（无论是否在同一个目录）
- **Private** 符号：仅在当前物理文件中可见

```cpp
// 文件 1: src/math/core.ch
module utils.math;

public func add(int a, int b) => a + b;

// 文件 2: src/math/geom.ch
module utils.math;

public func distance(Point p1, Point p2) { ... }
```

## 4. 模块导入

### 4.1 基本导入

使用 `import` 关键字导入其他模块：

```cpp
// 导入整个模块
import utils.math;
import network.http;

// 使用导入的模块
int sum = utils.math.add(1, 2);
```

### 4.2 别名导入

使用 `as` 关键字为导入的模块指定别名：

```cpp
// 别名导入
import utils.math as math;
import network.http as http;

// 使用别名
int sum = math.add(1, 2);
```

### 4.3 C 头文件导入

使用 `import c` 语法导入 C 头文件：

```cpp
// 导入 C 头文件
import c "stdio.h";
import c "stdlib.h";

// 使用 C 函数
printf("Hello, World!\n");
```

### 4.4 再导出 (Re-export)

C^ 支持通过 `public import` 将导入的符号重新导出，允许库作者提供统一的公共入口模块：

```cpp
// src/std/io.ch
module std.io;

// 导入并重新导出
// 用户 import std.io 后，可以直接使用 File 和 Console
public import std.io.impl.file;
public import std.io.impl.console;

// 带别名的重新导出
public import std.network.http as http;
```

**限制**：
- 编译器会检查并报告再导出链中的循环引用
- 如果多个 re-export 引入了同名符号，必须显式解决冲突

## 5. 可见性与导出

### 5.1 访问修饰符

C^ 提供了三种访问修饰符，用于控制成员的可见性：

| 修饰符     | 可见性                   |
| ---------- | ------------------------ |
| `public`   | 公开的，对所有模块可见   |
| `private`  | 私有的，只对当前文件可见 |
| `internal` | 内部的，只对当前模块可见 |

### 5.2 导出机制

C^ 采用基于 **访问修饰符 (`public`)** 的隐式导出机制：

- **默认 (Internal)**：如果不加修饰符，顶层符号（类、函数、变量）默认为 `internal`，即仅在**当前模块**内可见
- **公开 (Public)**：使用 `public` 修饰的顶层符号，会自动被**导出**，对导入该模块的外部代码可见
- **私有 (Private)**：使用 `private` 修饰的顶层符号，仅在**当前文件**内可见

```cpp
// utils/math.ch
module utils.math;

// 导出符号 (Public)
// 外部代码 import utils.math 后可见
public func add(int a, int b) -> int => a + b;

// 内部符号 (Internal)
// 仅 utils.math 模块内部可见，外部不可见
func helper() -> void {
    // 辅助函数
}

// 私有符号 (Private)
// 仅当前文件可见
private const int MAGIC = 42;
```

### 5.3 成员可见性

类的成员可见性遵循以下规则：

- **`public`**：对所有模块可见
- **`protected`**：对当前模块和子类可见
- **`private`**：只对当前类内部可见

```cpp
// utils/person.ch
module utils.person;

public class Person {
    // 私有成员，只对类内部可见
    private string _name;
    
    // 受保护成员，对类内部和子类可见
    protected int _age;
    
    // 公开成员，对所有模块可见
    public func init(string name, int age) {
        _name = name;
        _age = age;
    }
    
    public func get_name() -> string {
        return _name;
    }
    
    protected func set_age(int age) {
        _age = age;
    }
}
```

## 6. 模块依赖

### 6.1 依赖关系

- **直接依赖**：模块 A 导入模块 B，模块 A 直接依赖模块 B
- **间接依赖**：模块 A 导入模块 B，模块 B 导入模块 C，模块 A 间接依赖模块 C

### 6.2 依赖解析

编译器会自动解析模块依赖关系，按照以下规则：

1. 从当前模块开始，收集所有直接导入的模块
2. 对每个导入的模块，递归收集其依赖的模块
3. 构建依赖图，确保依赖关系的正确性
4. 按照依赖顺序编译模块

### 6.3 循环依赖

C^ **允许**同一个逻辑模块内的文件之间存在循环引用，但**禁止**跨逻辑模块的循环依赖：

```cpp
// 错误：跨模块循环依赖
// A.ch
module A;
import B;

// B.ch
module B;
import A;
```

**避免循环依赖的方法**：
- 重构代码，提取共同依赖到新模块
- 使用接口或抽象类解耦
- 重新设计模块边界

### 6.4 与 C/C++ 的互操作

#### 6.4.1 导入 C 头文件

C^ 编译器支持直接解析 C 头文件，将其虚拟为模块：

```cpp
// 引入 C 头文件
import c "stdio.h";

func main() {
    c.printf(c"Hello C World\n");
}
```

#### 6.4.2 导出给 C/C++

使用 `extern "C"` 修饰符可导出 C ABI 符号：

```cpp
extern "C" public func my_api() { ... }
```

#### 6.4.3 头文件生成策略

C^ 编译器支持从 `.ch` 源码自动生成头文件：
- **生成模式**：`hatc --emit-header my_lib.ch`
- **输出**：
  - `my_lib.h`：仅包含 `extern "C"` 的 C 接口
  - `my_lib.hh`：包含 C++ 类映射（如果启用了 C++ Interop）

## 7. 模块编译

### 7.1 单个模块编译

编译单个模块文件：

```bash
# 编译单个模块文件
chc src/main.ch -o main.exe
```

### 7.2 多模块编译

编译多个模块文件：

```bash
# 编译多个模块文件
chc src/utils/math.ch src/utils/string.ch src/main.ch -o app.exe

# 使用通配符
chc src/**/*.ch -o app.exe
```

### 7.3 模块输出

编译器可以将模块编译为目标文件，供其他模块使用：

```bash
# 编译为目标文件
chc -c src/utils/math.ch -o math.o
chc -c src/utils/string.ch -o string.o

# 链接目标文件
chc math.o string.o src/main.ch -o app.exe
```

## 8. 标准库模块

C^ 提供了丰富的标准库模块，用于常见的功能：

### 8.1 核心标准库

| 模块名           | 功能描述                           |
| ---------------- | ---------------------------------- |
| `std.core`       | 核心语言功能，如基本类型、内存管理 |
| `std.io`         | 输入输出操作                       |
| `std.math`       | 数学函数                           |
| `std.string`     | 字符串操作                         |
| `std.collection` | 集合类型，如数组、列表、映射       |
| `std.net`        | 网络操作                           |
| `std.thread`     | 线程和并发                         |
| `std.time`       | 时间操作                           |
| `std.file`       | 文件系统操作                       |
| `std.regex`      | 正则表达式                         |

### 8.2 使用标准库

```cpp
// 导入标准库模块
import std.io;
import std.math;
import std.string;

// 使用标准库
func main() -> int {
    std.io.println("Hello, World!");
    double pi = std.math.pi();
    string text = "  Hello  ";
    string trimmed = std.string.trim(text);
    return 0;
}
```

## 9. 模块最佳实践

### 9.1 模块设计原则

1. **单一职责**：每个模块应该只负责一个功能领域
2. **模块大小**：模块应该保持合理大小，通常不超过 10-20 个文件
3. **模块边界**：明确模块的职责边界，避免模块间过度耦合
4. **命名规范**：模块名应该清晰、描述性，使用小写字母和点分隔
5. **目录结构**：模块目录结构应该与模块名对应

### 9.2 可见性最佳实践

1. **最小可见性原则**：默认使用最小的可见性，只在必要时扩大可见范围
2. **公开接口**：只将必要的类型和函数声明为 `public`
3. **内部实现**：将实现细节保持为 `internal` 或 `private`
4. **文件私有**：对于仅在单个文件中使用的辅助函数，使用 `private`

### 9.3 依赖管理

1. **减少依赖**：尽量减少模块依赖，特别是跨系统的依赖
2. **依赖方向**：依赖应该从具体模块指向抽象模块
3. **依赖注入**：使用依赖注入模式减少模块间耦合
4. **接口隔离**：使用接口隔离不同模块的实现细节

## 10. 模块与编译单元

### 10.1 编译单元

- **编译单元**：C^ 中的编译单元是指一次编译操作处理的所有文件
- **模块与编译单元**：一个编译单元可以包含多个模块
- **模块边界**：模块边界在编译时由编译器处理，与编译单元无关

### 10.2 增量编译

C^ 编译器支持增量编译，只重新编译修改过的模块：

- 编译器会为每个模块生成中间表示
- 当模块内容变化时，只重新编译该模块及其依赖的模块
- 增量编译可以显著提高编译速度

## 11. 模块版本管理

### 11.1 版本控制

模块版本管理建议：

- 使用语义化版本号（Major.Minor.Patch）
- 在模块名中包含版本信息，如 `utils.math.v2`
- 提供向后兼容的 API
- 明确标记废弃的 API

### 11.2 版本迁移

模块版本迁移策略：

1. 保持旧版本模块可用
2. 提供新版本模块的并行实现
3. 在文档中说明版本差异和迁移指南
4. 逐步废弃旧版本模块

## 12. 模块示例

### 12.1 基础模块示例

```cpp
// src/utils/math.ch
module utils.math;

/**
 * Adds two integers.
 * 
 * @param a The first integer.
 * @param b The second integer.
 * @return The sum of the two integers.
 */
public func add(int a, int b) -> int {
    return a + b;
}

/**
 * Multiplies two integers.
 * 
 * @param a The first integer.
 * @param b The second integer.
 * @return The product of the two integers.
 */
public func multiply(int a, int b) -> int {
    return a * b;
}

// 内部辅助函数
private func validate_range(int value, int min, int max) -> bool {
    return value >= min && value <= max;
}
```

### 12.2 模块导入示例

```cpp
// src/main.ch
module main;

// 导入模块
import utils.math;
import std.io;

func main() -> int {
    int sum = utils.math.add(1, 2);
    int product = utils.math.multiply(3, 4);
    
    std.io.println("Sum: " + sum);
    std.io.println("Product: " + product);
    
    return 0;
}
```

### 12.3 多文件模块示例

```cpp
// src/utils/string.ch
module utils.string;

public func length(string s) -> int {
    return s.length;
}

public func concat(string a, string b) -> string {
    return a + b;
}

// src/utils/string/trim.ch
module utils.string;

public func trim(string s) -> string {
    // 实现 trim 逻辑
    return s;
}

public func trim_start(string s) -> string {
    // 实现 trim_start 逻辑
    return s;
}

public func trim_end(string s) -> string {
    // 实现 trim_end 逻辑
    return s;
}
```

## 13. 模块系统优势

C^ 模块系统相比传统 C/C++ 头文件系统的优势：

### 13.1 编译速度

- **减少重复编译**：模块只编译一次，多次导入时使用编译结果
- **增量编译**：只重新编译修改的模块
- **并行编译**：模块间可以并行编译

### 13.2 代码组织

- **清晰的模块边界**：明确的模块声明和导入语法
- **合理的可见性控制**：通过访问修饰符控制成员可见性
- **与目录结构对应**：模块名通常与目录结构对应，便于导航

### 13.3 类型安全

- **显式导入**：需要显式导入使用的模块，避免隐式依赖
- **模块隔离**：模块间相互隔离，减少命名冲突
- **编译时检查**：编译器会检查模块依赖的正确性

### 13.4 工具支持

- **IDE 支持**：现代 IDE 可以更好地理解模块结构，提供智能提示
- **重构工具**：模块系统便于代码重构和重组织
- **文档生成**：模块结构便于自动生成文档

## 14. 总结

C^ 语言的模块系统是一个现代化的代码组织机制，提供了：

- **清晰的模块声明**：使用 `module` 关键字声明模块
- **灵活的模块导入**：支持整个模块导入和别名导入
- **精确的可见性控制**：通过访问修饰符控制成员可见性
- **隐式导出机制**：使用 `public` 修饰符自动导出成员
- **C 头文件兼容**：支持导入 C 头文件
- **标准库模块**：丰富的标准库模块

C^ 的模块系统设计兼顾了编译速度、代码组织、类型安全和工具支持，为大型项目提供了良好的代码组织基础。通过合理使用模块系统，可以编写更清晰、更可维护、更可扩展的代码。