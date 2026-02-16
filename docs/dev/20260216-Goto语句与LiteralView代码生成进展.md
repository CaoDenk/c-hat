# Goto 语句与 LiteralView 代码生成进展

## 日期
2026-02-16

## 当前进展

### 1. Goto 语句支持

#### 1.1 新增文件
- **src/ast/statements/GotoStmt.h** - Goto 语句 AST 节点
- **src/ast/statements/LabelStmt.h** - 标签语句 AST 节点

#### 1.2 修改文件
- **src/ast/NodeType.h** - 添加 GotoStmt 和 LabelStmt 节点类型
- **src/ast/AstNodes.h** - 包含新的语句头文件
- **src/parser/Parser.cpp** - 实现 goto 语句和标签的解析

#### 1.3 功能实现
- ✅ **标签声明解析**：`label:` 格式
- ✅ **Goto 语句解析**：`goto label;` 格式
- ✅ **标签与标识符区分**：使用 saveState/restoreState 尝试解析标签，不是标签则回滚解析为普通表达式
- ✅ **测试文件**：创建了 `tests/test_goto.ch`，解析成功！

### 2. LiteralView 类型与隐式转换

#### 2.1 类型兼容性规则
- **LiteralViewType.cpp**：修改了 `isCompatibleWithImpl`
- **兼容性规则**：
  - LiteralView 与其他 LiteralView 兼容
  - LiteralView 与 Slice<byte> 兼容
  - 结合 Type 基类的 Readonly 处理，自动兼容 byte![]（Readonly<Slice<byte>>）

#### 2.2 类型兼容性检查方向修复
- **SemanticAnalyzer.cpp**：
  - 修复了赋值表达式：改为 `rightType.isCompatibleWith(leftType)`
  - 修复了函数参数：改为 `argType.isCompatibleWith(paramType)`

#### 2.3 测试验证
- 创建了 `tests/test_literalview_simple.ch`
- ✅ 语义分析成功！`byte![] s = "hello";` 通过类型检查

### 3. LiteralView 代码生成（LLVM）

#### 3.1 LiteralView 结构体类型定义
- **LLVMCodeGenerator.cpp**：新增 `getLiteralViewType()` 方法
- **结构体布局**：
  ```cpp
  struct LiteralView {
    i8* ptr;   // 指向字符串数据的指针
    i64 len;    // 字符串长度（不含 null 终止符）
  };
  ```

#### 3.2 字符串字面量代码生成
- **LLVMCodeGenerator.cpp**：修改了 `createStringLiteral()` 方法
- **实现逻辑**：
  1. 创建 .rodata 段的全局字符串常量（带 null 终止符）
  2. 使用 GEP 获取指向第一个字符的指针
  3. 创建 LiteralView 结构体
  4. 设置 ptr 字段为字符串指针
  5. 设置 len 字段为字符串长度（str.size()，不含 null 终止符）

#### 3.3 头文件更新
- **LLVMCodeGenerator.h**：添加了 `getLiteralViewType()` 方法声明

### 4. 设计文档

#### 4.1 Goto 语句设计
- 创建了 `docs/design/Goto语句设计.md`
- **设计规则**：
  - ✅ 必须在同一函数内
  - ✅ 支持双向跳转（向前/向后）
  - ✅ 禁止跳过变量初始化
  - ✅ 禁止跳入 try 块
- 提供了推荐用法和对比分析

---

## 技术细节

### LiteralView 的 LLVM IR 示例

```llvm
%LiteralView = type { i8*, i64 }

@.str = private unnamed_addr constant [6 x i8] c"hello\00", align 1

define i32 @test() {
entry:
  %strptr = getelementptr inbounds ([6 x i8], [6 x i8]* @.str, i32 0, i32 0
  %literalview = insertvalue %LiteralView undef, i8* %strptr, 0
  %literalview1 = insertvalue %LiteralView %literalview, i64 5, 1
  ret i32 0
}
```

---

## 下一步计划

### 短期目标（1-2天）
1. **完善 LiteralView 的成员访问代码生成**
   - [ ] 实现 `s.ptr` 和 `s.len` 的访问
   - [ ] 确保这些访问是只读的

2. **实现 LiteralView 到 byte![] 的隐式转换代码生成**
   - [ ] 在赋值和参数传递时自动转换
   - [ ] 保持零拷贝

3. **完善 Goto 语句的语义分析**
   - [ ] 检查标签是否在同一函数内
   - [ ] 检查标签是否已声明
   - [ ] 检查是否跳过变量初始化

### 中期目标（1-2周）
1. **完善 Goto 语句的代码生成**
   - [ ] 在 LLVMCodeGenerator 中添加 GotoStmt 和 LabelStmt 的支持
   - [ ] 生成正确的基本块和分支指令

2. **完善字符串系统**
   - [ ] 实现 string 类型
   - [ ] 实现 string_view 类型
   - [ ] 实现字符串拼接等操作

---

## 已修改/新增文件清单

### 新增文件
1. `src/ast/statements/GotoStmt.h`
2. `src/ast/statements/LabelStmt.h`
3. `tests/test_goto.ch`
4. `tests/test_literalview_simple.ch`
5. `tests/test_literalview_slice.ch`
6. `docs/design/Goto语句设计.md`
7. `docs/dev/20260216-LiteralView与只读类型系统进展.md`

### 修改文件
1. `src/ast/NodeType.h` - 添加 GotoStmt 和 LabelStmt
2. `src/ast/AstNodes.h` - 包含新的语句头文件
3. `src/parser/Parser.cpp` - 实现 goto 语句和标签解析
4. `src/types/LiteralViewType.cpp` - 添加与 byte[] 的兼容性
5. `src/semantic/SemanticAnalyzer.cpp` - 修复兼容性检查方向
6. `src/llvm/LLVMCodeGenerator.h` - 添加 getLiteralViewType() 声明
7. `src/llvm/LLVMCodeGenerator.cpp` - 实现 LiteralView 代码生成

---

## 总结

本次完成了以下核心功能：
1. ✅ **Goto 语句解析**：完整支持 goto 语句和标签的解析
2. ✅ **LiteralView 类型兼容性**：实现了 LiteralView 到 byte![] 的隐式转换（语义分析）
3. ✅ **类型兼容性检查方向修复**：修复了赋值和参数传递的兼容性检查
4. ✅ **LiteralView 代码生成**：在 LLVM 中生成了 LiteralView 的结构体和字符串字面量
5. ✅ **Goto 设计文档**：详细定义了 goto 的语法、语义和约束

所有核心功能都已实现！
