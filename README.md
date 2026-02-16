# C^ (C-Hat) 编译器开发项目

## 项目简介

C^ (C-Hat) 是一门旨在取代 C++ 的现代系统编程语言，追求极致的零成本抽象，拒绝垃圾回收和重型运行时。它保留了 C 的底层控制力，同时引入了现代语言的严谨性和安全性。

本项目使用AI辅助开发C^语言的完整编译器实现。

## 核心设计理念

- **性能优先**：不低于 C++ 的性能
- **安全性**：空安全、边界检查、类型安全
- **现代语法**：简洁、清晰、无歧义
- **零成本抽象**：编译期展开、静态多态
- **全栈能力**：从操作系统到高层业务

## 语言特性

### 基础类型

- **整数**：`byte`/`sbyte` (8位), `short`/`ushort` (16位), `int`/`uint` (32位), `long`/`ulong` (64位)
- **字符**：`char` (32位 Unicode 码点)
- **浮点**：`float`, `double`, `fp16` (半精度), `bf16` (Brain Float)
- **布尔**：`bool`
- **字符串**：`string_view` (标准库类型，UTF-8)

### 变量声明

```cpp
// 显式类型声明
int counter = 0;
byte mask = 0xFF;
char ch = '中';

// 类型推导
var total = 100;     // int, 可变
let limit = 50;      // int, 不可变

// 常量
const int MAX_RETRIES = 3;
```

### 函数定义

```cpp
// 标准函数定义
func calculate_area(int width, int height) -> int {
    return width * height;
}

// 单表达式函数
func add(int a, int b) -> int => a + b;

// 多返回值
func get_status() -> (int, string) {
    return (200, "OK");
}
```

### 控制流

```cpp
// 条件语句 (必须带大括号)
if (x > 0) {
    print("Positive");
} else {
    print("Non-positive");
}

// Match 表达式 (取代 switch)
string status_text = match (code) {
    200 => "OK",
    404 => "Not Found",
    _   => "Unknown"
};

// 循环
for (int i = 0; i < 10; i++) {
    // ...
}

while (running) {
    // ...
}
```

### 数组与切片

```cpp
// 栈上固定数组
int[5] arr1 = [1, 2, 3, 4, 5];

// 栈数组推导
int[$] arr2 = [1, 2, 3]; 

// 切片 (Slice)
var s = "hello"; // 类型为 byte![]

// 传递切片视图
int total = sum_array(numbers[0..3]);
```

### 指针系统

```cpp
// 指针操作
var x = 42;
var ptr = &x^;      // 取地址，非空指针
int^ p = &x;        // 可空指针
ptr^ = 100;         // 解引用

// 成员访问
obj->member;         // 指针成员访问
obj^.member;         // 解引用后访问
```

### 类与面向对象

```cpp
class Rectangle {
    int width;
    int height;
    
    func area() -> int {
        return width * height;
    }
}

class Derived extends Base {
    func getValue() -> int {
        return super.getValue() + 1;
    }
}
```

### 泛型系统

```cpp
// 泛型函数
func <T> identity(T value) -> T {
    return value;
}

// 约束
func <T> add(T a, T b) -> T requires (T a, T b) {
    return a + b;
}

// 变参泛型
func <...Ts> print_all(Ts... args) {
    print(args...);
}
```

### Lambda 表达式

```cpp
// 简写语法
var f = x => x + 1;

// 完整语法
var g = [](int x) -> int {
    return x * 2;
};

// 多语句
var h = [](int x) {
    var y = x + 1;
    return y * 2;
};
```

## 编译器架构

### 前端组件

1. **词法分析器 (Lexer)**
   - 识别C^的所有token
   - 处理关键字、操作符、字面量
   - 生成token流

2. **语法分析器 (Parser)**
   - 基于EBNF文法的递归下降解析器
   - 构建抽象语法树(AST)
   - 错误恢复机制

3. **语义分析器 (Semantic Analyzer)**
   - 类型检查
   - 符号表管理
   - 作用域分析
   - 泛型实例化

### 后端组件

4. **中间代码生成器 (IR Generator)**
   - 将AST转换为中间表示
   - 支持SSA形式
   - 优化准备

5. **代码生成器 (Code Generator)**
   - 生成目标代码
   - 寄存器分配
   - 指令选择

6. **优化器 (Optimizer)**
   - 常量折叠
   - 死代码消除
   - 循环优化
   - 内联展开

## 开发环境

- **操作系统**: Windows/Linux/macOS
- **编程语言**: C++23
- **构建工具**: CMake 3.16+
- **测试框架**: GoogleTest
- **版本控制**: Git

## 项目结构

```
.
├── src/
│   ├── lexer/          # 词法分析器
│   ├── parser/         # 语法分析器
│   ├── semantic/       # 语义分析器
│   ├── ast/            # 抽象语法树
│   ├── codegen/       # 代码生成器
│   └── main.cpp       # 测试入口
├── docs/
│   ├── design/         # 设计文档
│   ├── spec/          # 规范文档
│   └── dev/           # 开发文档
├── tests/            # 测试用例
└── examples/         # 示例代码
```

## 开发路线图

### 第一阶段：基础功能 ✅
- [x] 词法分析器
- [x] 基础语法分析器
- [x] 变量声明解析
- [x] 函数声明解析
- [x] 表达式解析
- [x] 控制流语句解析
- [x] 类定义解析
- [x] 泛型基础支持
- [x] Lambda表达式解析

### 第二阶段：语义分析 🚧
- [ ] 完整类型系统
- [ ] 符号表管理
- [ ] 作用域分析
- [ ] 类型检查
- [ ] 泛型实例化
- [ ] 错误诊断

### 第三阶段：代码生成 📋
- [ ] 中间表示设计
- [ ] AST到IR转换
- [ ] 基础代码生成
- [ ] 寄存器分配
- [ ] 目标代码输出

### 第四阶段：优化 📋
- [ ] 基础优化
- [ ] 高级优化
- [ ] 性能分析
- [ ] 优化反馈

## AI辅助开发策略

1. **增量式开发**
   - 从简单的表达式开始
   - 逐步添加语言特性
   - 每个阶段都进行充分测试

2. **测试驱动开发**
   - 为每个功能编写测试用例
   - 使用设计文档验证正确性
   - 回归测试确保稳定性

3. **代码审查**
   - AI生成的代码需要人工审查
   - 关注代码质量和安全性
   - 遵循编码规范

4. **文档同步**
   - 保持设计文档与代码同步
   - 记录设计决策
   - 提供API文档

## 测试策略

1. **单元测试**
   - 每个模块独立测试
   - 边界条件测试
   - 错误处理测试

2. **集成测试**
   - 完整编译流程测试
   - 多文件编译测试
   - 标准库测试

3. **回归测试**
   - 使用测试用例验证
   - 性能基准测试
   - 兼容性测试

## 贡献指南

1. Fork项目
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

## 许可证

MIT License

## 联系方式

如有问题或建议，请提交Issue或Pull Request。
