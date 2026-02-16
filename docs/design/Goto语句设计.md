# Goto 语句设计

## 概述
Goto 语句是一种低级控制流语句，允许直接跳转到代码中的标签位置。本设计文档定义了 c-hat 语言中 goto 语句的语法、语义和约束。

## 设计原则
- **安全性**：提供合理的约束，避免滥用
- **实用性**：保留必要的灵活性，用于错误处理、状态机等场景
- **一致性**：与现有控制流结构（for、while、try/catch）协调

## 语法

### 标签声明
```
<标识符>:
```

示例：
```
end:
error:
loop_start:
```

### Goto 语句
```
goto <标识符>;
```

示例：
```
goto end;
goto error;
```

## 语义约束

### 1. 函数内约束 ✅
- **必须遵守**：goto 和对应的 label 必须在同一个函数内
- **原因**：
  - LLVM IR 的基本块（BasicBlock）必须属于同一个函数
  - 跨函数跳转破坏了函数的封装性和调用约定
  - 难以维护和调试
- **错误示例**：
```
func foo() {
  goto bar_label;  // ❌ 错误：bar_label 在另一个函数中
}

func bar() {
bar_label:
  return;
}
```

### 2. 跳转方向 ✅
- **允许**：既可以向前跳，也可以向后跳
- **原因**：
  - 向后跳可用于实现简单循环（虽然推荐使用 for/while）
  - 向前跳可用于错误处理、提前退出等场景
- **示例**：
```
// 向后跳（循环）
func sum(int n) -> int {
  int i = 1;
  int total = 0;
loop:
  if (i > n) goto end;
  total = total + i;
  i = i + 1;
  goto loop;
end:
  return total;
}

// 向前跳（错误处理）
func process() -> int {
  int result = doSomething();
  if (result < 0) goto error;
  
  result = doAnotherThing();
  if (result < 0) goto error;
  
  goto success;
  
error:
  printError(result);
  return -1;
  
success:
  return 0;
}
```

### 3. 变量声明约束 ✅
- **禁止**：goto 不能跳过带有初始化的变量声明
- **原因**：
  - 跳过初始化会导致未定义行为
  - 保证变量在使用前已经正确初始化
- **错误示例**：
```
func bad() {
  goto skip;  // ❌ 错误：跳过了 x 的初始化
  int x = 42;
skip:
  x = 100;  // x 未初始化
}
```
- **允许**：可以跳过没有初始化的变量声明，或在声明后跳转
```
func okay1() {
  int x;  // 无初始化
  goto skip;
skip:
  x = 100;  // ✅ 可以
}

func okay2() {
  int x = 42;
  goto skip;  // ✅ 可以，在声明后跳转
skip:
  x = 100;
}
```

### 4. Try/Catch 约束 ✅
- **禁止**：goto 不能跳入 try 块内部
- **原因**：
  - 破坏异常处理的语义
  - 难以维护异常栈的状态
- **允许**：
  - 可以从 try 块内部跳转到 try 块外部（跳出 try）
  - 可以在 try 块内部跳转
  - 可以在 catch 块内部跳转
- **示例**：
```
func example() {
try:
  goto inside;  // ❌ 错误：不能跳入 try 块
  
  try {
    goto outside;  // ✅ 可以：跳出 try 块
    goto inner;    // ✅ 可以：try 块内部跳转
  inner:
    throw Error();
  } catch (Error e) {
    goto catch_end;  // ✅ 可以：catch 块内部跳转
  catch_end:
  }
  
outside:
  return;
}
```

## 设计对比

### 与 C/C++ 的对比
| 特性           | C/C++       | c-hat        |
| -------------- | ----------- | ------------ |
| 函数内约束     | ✅           | ✅            |
| 跳转方向       | 双向        | 双向         |
| 变量初始化跳过 | 警告/未定义 | 编译错误     |
| Try/Catch 约束 | 无          | 禁止跳入 try |

### 与 Go 的对比
| 特性           | Go       | c-hat    |
| -------------- | -------- | -------- |
| 函数内约束     | ✅        | ✅        |
| 跳转方向       | 双向     | 双向     |
| 变量初始化跳过 | 编译错误 | 编译错误 |

### 与 Rust 的对比
| 特性      | Rust                | c-hat                           |
| --------- | ------------------- | ------------------------------- |
| Goto 存在 | ❌ 无                | ✅ 有                            |
| 替代方案  | loop/break/continue | 保留 goto + loop/break/continue |

## 推荐用法

### 1. 错误处理（多出口场景）
```
func processFile(string path) -> int {
  File^ f = openFile(path);
  if (f == null) goto error_open;
  
  Buffer^ buf = allocBuffer(1024);
  if (buf == null) goto error_buf;
  
  int result = readFile(f, buf);
  if (result < 0) goto error_read;
  
  result = processBuffer(buf);
  
  freeBuffer(buf);
  closeFile(f);
  return result;
  
error_read:
  freeBuffer(buf);
error_buf:
  closeFile(f);
error_open:
  return -1;
}
```

### 2. 状态机
```
func stateMachine() {
  int state = 0;
  
state_0:
  if (condition()) goto state_1;
  goto state_end;
  
state_1:
  if (anotherCondition()) goto state_2;
  goto state_0;
  
state_2:
  goto state_end;
  
state_end:
  return;
}
```

### 3. 不推荐用法
- 不要用 goto 实现普通循环（用 for/while）
- 不要过度使用 goto，保持代码清晰
- 避免嵌套过深的 goto 跳转

## 总结
本设计提供了一套平衡的 goto 语句规则：
- ✅ 必须在同一函数内
- ✅ 支持双向跳转
- ✅ 禁止跳过变量初始化
- ✅ 禁止跳入 try 块
- ✅ 允许从 try 块跳出

这套规则既保留了 goto 的实用性，又避免了其滥用带来的问题。
