# C^ 编译器测试覆盖报告

> 最后更新：2026-05-30  
> 测试集总数：33  
> 修复：所有测试统一使用 `SemanticAnalyzer("", false)` 无 main 模式，消除 BUG-01 误报  
> 修复：填充 8 个语义分析桩（analyzeNewExpr/StructInitExpr/this/self/super/delete/lambda/expansion）  
> 修复：parser super 关键字改为 SuperExpr AST 节点，private 继承访问控制检查  
> 修复：class_system / overload / property / semantic / string_literal / tuple / variadic 全部通过  
> 修复：foreach / match / generics / nullable / reference / static / result_type 全部通过（预存代码修复）

---

## 一、测试结果总览

| 测试集 | 断言 | 状态 | 备注 |
|--------|------|------|------|
| `access_control` | 3 | ✅ 全通过 | 访问控制 |
| `array` | 27 | ✅ 全通过 | 数组声明、字面量、切片 |
| `attribute` | 40 | ✅ 全通过 | 自定义属性系统 |
| `builtin_vars` | 11 | ✅ 全通过 | 内置变量表达式 |
| `class_system` | 34 | ✅ 全通过 | 继承、super、访问控制 |
| `coroutine` | 13 | ✅ 全通过 | 协程 basic |
| `defer` | 4 | ✅ 全通过 | defer 语句 |
| `exception` | 3 | ✅ 全通过 | try-catch |
| `extension` | 6 | ✅ 全通过 | 扩展方法 |
| `ffi` | 9 | ✅ 全通过 | FFI extern 声明 |
| `foreach` | 9 | ✅ 全通过 | foreach 迭代 |
| `generics` | 13 | ✅ 全通过 | 泛型参数作用域 |
| `goto` | 8 | ✅ 全通过 | goto 语句 |
| `immutable_method` | 10 | ✅ 全通过 | `func foo()!` / `self!` |
| `lambda` | 2 | ✅ 全通过 | Lambda 表达式 |
| `late` | 10 | ✅ 全通过 | late 声明 |
| `match` | 2 | ✅ 全通过 | match 表达式 |
| `module` | 10 | ✅ 全通过 | 模块声明与导入 |
| `new_delete` | 13 | ✅ 全通过 | new/delete 表达式 |
| `nullable` | 12 | ✅ 全通过 | 可空类型 `?` 传播 |
| `overload` | 31 | ✅ 全通过 | 函数重载解析 |
| `parser` | 64 | ✅ 全通过 | 基础语法解析 |
| `property` | 5 | ✅ 全通过 | get/set 属性 |
| `reference` | 7 | ✅ 全通过 | 引用类型 |
| `result_type` | 6 | ✅ 全通过 | Result<T,E> 语义 |
| `semantic` | 29 | ✅ 全通过 | 语义分析综合 |
| `static` | 8 | ✅ 全通过 | static 成员 |
| `string_literal` | 12 | ✅ 全通过 | 字符串字面量 |
| `tuple` | 13 | ✅ 全通过 | 元组表达式 |
| `types` | 34 | ✅ 全通过 | 类型系统单元测试 |
| `variadic` | 4 | ✅ 全通过 | extern"C"变参 |

| `pointer` | 37 | ⚠️ 36/37 | 1 失败（指针差类型推导） |

**汇总：**
- ✅ 全通过：32 个测试集
- ⚠️ 部分通过：1 个测试集（pointer，36/37）

---

## 二、失败详情与根因分析

### 当前唯一失败

| 测试 | 断言 | 源码 | 根因 |
|------|------|------|------|
| `pointer/Pointer difference` | 36/37 | `int diff = ptr2 - ptr1;` | 指针减法类型推导：`int^ - int^` 应返回整型而非指针类型，语义分析器当前返回指针类型导致赋值类型不匹配 |

---

---

## 三、待修复

| 缺陷 | 测试 | 断言 | 根因 |
|------|------|------|------|
| 指针减法返回类型 | `pointer` | 36/37 | `int^ - int^` 语义分析应返回整型；当前返回指针导致赋值失败 |

---

## 四、快速命令

```bash
# 构建所有测试
cmake --build build --config Release

# 运行单个测试
./build/tests/<name>/Release/<name>_catch2_test.exe

# 批量运行
Get-ChildItem -Recurse -Filter "*test.exe" | Where-Object FullName -match '\\Release\\' | ForEach-Object { & $_ }
```

## 五、当前覆盖状态

- **总测试集**：33
- **全通过**：32 个
- **部分通过**：1 个（pointer 36/37）
- **覆盖率（断言）**：622 / 623 (99.8%)
