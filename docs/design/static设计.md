# C^ static 关键字设计文档

## 1. 设计目标

C^ 的 static 关键字设计旨在：
1. **减少开发者踩坑**：避免 C/C++ 中 static 语义过载导致的混淆
2. **明确语义**：一个关键字只做一件事，不赋予多重含义
3. **与模块系统配合**：明确模块级别的可见性和生命周期

---

## 2. 设计决策：分离 static 的多重语义

C/C++ 中 static 有**多重含义**，这是开发者最容易踩坑的地方：
- 函数内 static → 静态局部变量
- 模块级 static → 内部链接（文件作用域）
- 类内 static → 静态成员

**C^ 的解决方案：使用不同的关键字！**

| 语义                   | C++ 关键字 | C^ 关键字  | 说明                   |
| ---------------------- | ---------- | ---------- | ---------------------- |
| 静态局部变量           | `static`   | `static`   | ✅ 保留，语义最清晰     |
| 内部链接（文件作用域） | `static`   | `internal` | ✅ 用 internal 明确表示 |
| 类静态成员             | `static`   | `static`   | ✅ 保留，语义清晰       |

---

## 3. C^ 中 static 的用途

### 3.1 用途 1：静态局部变量（函数内）

**语法**：
```cpp
func counter() -> int {
    static int count = 0;  // 静态局部变量
    count++;
    return count;
}
```

**语义**：
- 变量在程序启动时初始化一次
- 生命周期是整个程序运行期间
- 作用域仅限于函数内部
- 每次调用函数共享同一个变量

**代码生成**：
```llvm
@counter.count = internal global i32 0  ; 模块级全局变量

define i32 @counter() {
entry:
  %0 = load i32, i32* @counter.count, align 4
  %1 = add nsw i32 %0, 1
  store i32 %1, i32* @counter.count, align 4
  ret i32 %1
}
```

---

### 3.2 用途 2：类静态成员

**语法**：
```cpp
class Counter {
public:
    static int total_count;  // 静态成员变量
    static func reset() {    // 静态成员函数
        total_count = 0;
    }
}

// 在模块级初始化静态成员
int Counter::total_count = 0;
```

**语义**：
- 静态成员变量属于类，不属于实例
- 所有实例共享同一个静态成员
- 静态成员函数没有 self 参数，不能访问非静态成员

---

## 4. 模块级变量的可见性：用 internal，禁止 static！

**C^ 设计规则 1：模块级变量默认是 internal 的！**

**C^ 设计规则 2：模块级绝对禁止使用 static 关键字！** ⚠️

### 4.1 默认可见性：internal

**语法**：
```cpp
// module_a.ch

// 模块级变量，默认是 internal
// 只在当前模块内可见，其他模块无法访问
int module_internal_var = 42;

// 模块级函数，默认也是 internal
func module_internal_func() {
    // ...
}
```

**语义**：
- 模块级声明（变量、函数、类）默认是 `internal`
- 只在当前模块内可见
- 其他模块无法导入和使用

### 4.2 导出可见性：public

如果要让其他模块可以访问，需要显式标记 `public`：

**语法**：
```cpp
// module_a.ch

// 显式 public，其他模块可以导入
public int public_var = 42;

public func public_func() {
    // ...
}

public class PublicClass {
    // ...
}
```

**语义**：
- `public` 声明可以被其他模块导入
- 没有 `public` 就是 `internal`
- **不需要 static 来表示内部链接！**

### 4.3 模块级禁止 static！

**C^ 编译器在模块级遇到 static 会直接报错！**

```cpp
// C^ - 模块级禁止使用 static！ ❌
static int x = 42;  // 编译器错误！
// Error: static cannot be used at module level
// Use default visibility (internal) or public instead
```

**正确的 C^ 写法：**
```cpp
// C^ - 清晰明确 ✅
int x = 42;           // 模块级，默认 internal（当前模块可见）
public int y = 42;    // 模块级，public（其他模块可导入）

func f() {
    static int z = 0;  // 函数内，静态局部变量（允许！）
}
```

### 4.4 为什么这样设计？

**彻底避免 C++ 的坑：**
```cpp
// C++ - 容易混淆
static int x = 42;  // 这是内部链接？还是静态变量？
// 在函数外：内部链接
// 在函数内：静态局部变量
```

**C^ 设计理念：一个关键字只做一件事！**
- `static` → 只用于**静态局部变量**和**类静态成员**
- 模块级可见性 → 默认 `internal` 或显式 `public`
- 没有歧义！

---

## 5. 完整示例对比

### 5.1 C++ 写法（容易混淆）

```cpp
// file.cpp

// static = 内部链接
static int file_local_var = 42;

// 没有 static = 外部链接
int global_var = 42;

int counter() {
    // static = 静态局部变量
    static int count = 0;
    return ++count;
}

class MyClass {
public:
    // static = 类静态成员
    static int static_member;
};
```

**问题**：同一个 `static` 关键字，在不同位置含义完全不同！

### 5.2 C^ 写法（清晰明确）

```cpp
// module.ch

// 默认 = internal（仅当前模块可见）
int module_local_var = 42;

// 显式 public = 其他模块可访问
public int exported_var = 42;

func counter() -> int {
    // static = 静态局部变量（含义明确）
    static int count = 0;
    count++;
    return count;
}

class MyClass {
public:
    // static = 类静态成员（含义明确）
    static int static_member;
}
```

**优点**：
- 模块级可见性通过 `public`/默认 `internal` 控制
- `static` 只用于静态局部变量和类静态成员
- 没有歧义，不易踩坑

---

## 6. AST 节点设计

### 6.1 存储修饰符信息

在相关 AST 节点中添加 `isStatic` 字段：

```cpp
// 在 VariableDecl 中
class VariableDecl : public Decl {
public:
    bool isStatic;  // 是否是静态变量
    // ...
};

// 在 FunctionDecl 中
class FunctionDecl : public Decl {
public:
    bool isStatic;  // 是否是静态函数
    // ...
};
```

### 6.2 解析 specifiers 字符串

当前 Parser 将 static 拼接到 `specifiers` 字符串中，需要改为解析为布尔字段：

```cpp
// 解析修饰符
bool isPublic = false;
bool isPrivate = false;
bool isStatic = false;

while (true) {
    if (match(TokenType::Public)) {
        isPublic = true;
    } else if (match(TokenType::Private)) {
        isPrivate = true;
    } else if (match(TokenType::Static)) {
        isStatic = true;
    } else {
        break;
    }
}

// 传递给 AST 构造函数
return std::make_unique<VariableDecl>(
    name, std::move(type), std::move(initializer),
    isPublic, isPrivate, isStatic);
```

---

## 7. 语义分析检查

### 7.1 模块级禁止 static 检查 ⚠️

```cpp
void SemanticAnalyzer::analyzeVariableDecl(VariableDecl* varDecl) {
    if (varDecl->isStatic) {
        // 检查是否在模块级（不在函数内也不在类内）
        if (!currentFunction_ && !currentClass_) {
            // 模块级禁止使用 static！
            error(
                "static cannot be used at module level\n"
                "Use default visibility (internal) or public instead", 
                *varDecl
            );
            return;
        }
    }
}
```

### 7.2 静态局部变量检查

```cpp
void SemanticAnalyzer::analyzeVariableDecl(VariableDecl* varDecl) {
    if (varDecl->isStatic && currentFunction_) {
        // 检查是否有初始化器
        if (!varDecl->initializer) {
            error("static variable must have an initializer", *varDecl);
        }
    }
}
```

### 7.3 静态成员函数检查

```cpp
void SemanticAnalyzer::analyzeFunctionDecl(FunctionDecl* funcDecl) {
    if (funcDecl->isStatic && currentClass_) {
        // 静态成员函数不能有 self 参数
        if (funcDecl->hasSelfParameter) {
            error("static member function cannot have self parameter", *funcDecl);
        }
    }
}
```

---

## 8. 代码生成

### 8.1 静态局部变量代码生成

```cpp
llvm::Value* LLVMCodeGenerator::generateVariableDecl(VariableDecl* varDecl) {
    if (varDecl->isStatic && currentFunction_) {
        // 生成唯一的全局变量名
        std::string globalName = currentFunctionName_ + "." + varDecl->name;
        
        // 创建模块级全局变量
        llvm::GlobalVariable* gv = new llvm::GlobalVariable(
            *module_,
            varType,
            false,  // not constant
            llvm::GlobalValue::InternalLinkage,
            initValue,
            globalName
        );
        
        return gv;
    }
    
    // 普通变量...
}
```

---

## 9. 最佳实践

✅ **推荐做法：**
- 模块级变量不需要写 `static`，默认就是 `internal`
- 要导出的模块级声明显式写 `public`
- 函数内需要持久化状态时用 `static` 局部变量
- 类成员属于类而不属于实例时用 `static`

❌ **避免做法：**
- 不要在模块级写 `static`（C^ 不需要！）
- 不要过度使用静态局部变量，会影响可测试性
- 静态成员函数不要尝试访问非静态成员

---

## 10. 总结

C^ 的 static 设计通过**分离关注点**来减少踩坑：
- 模块可见性 → `public` / 默认 `internal`
- 静态局部变量 → `static`
- 类静态成员 → `static`

这样每个关键字的职责单一且明确，开发者一看就知道是什么意思！
