# 从 LLVM IR 到可执行程序的完整流程

本文档详细介绍如何将 C^ 语言编译器生成的 LLVM IR 转换为最终可执行程序。

## 1. 编译流程概述

完整的编译流程分为以下几个阶段：

```
C^ 源代码 → 词法分析 → 语法分析 → 语义分析 → LLVM IR 生成 → 目标代码生成 → 链接 → 可执行程序
```

## 2. 当前编译器功能

目前 C^ 编译器 (`chc`) 支持以下功能：

| 功能 | 命令行选项 | 说明 |
|------|------------|------|
| 生成 LLVM IR | `--emit-llvm` | 生成 `.ll` 格式的 LLVM IR 文件 |
| 生成目标文件 | `--emit-obj` | 目前暂不可用，建议手动使用 llc |
| 生成汇编文件 | `--emit-asm` | 目前暂不可用，建议手动使用 llc |

## 3. Windows 平台完整流程

### 3.1 步骤 1：生成 LLVM IR

首先，使用编译器将 C^ 源代码编译为 LLVM IR：

```bash
cd d:\projects\CppProjs\c-hat
.\build\src\Debug\c_hat_parser.exe tests\hello_world.ch --emit-llvm --dump-ir
```

这会生成 `hello_world.ll` 文件。

### 3.2 步骤 2：使用 llc 生成目标文件

使用 LLVM 的 `llc` 工具将 LLVM IR 转换为目标文件：

```bash
llc -filetype=obj hello_world.ll -o hello_world.obj
```

如果没有 `llc` 在 PATH 中，可以使用 vcpkg 安装的 LLVM：

```bash
D:\vcpkg\installed\x64-windows\tools\llvm\llc.exe -filetype=obj hello_world.ll -o hello_world.obj
```

### 3.3 步骤 3：使用 link.exe 链接

使用 Microsoft Visual Studio 的链接器 `link.exe` 将目标文件链接为可执行程序：

```bash
link.exe hello_world.obj /OUT:hello_world.exe /SUBSYSTEM:CONSOLE msvcrt.lib
```

要在命令行中使用 `link.exe`，需要先设置 Visual Studio 环境变量：

```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

（注意：根据你的 Visual Studio 版本和安装路径调整路径）

### 3.4 步骤 4：运行可执行程序

```bash
hello_world.exe
```

输出应该是：
```
Hello, World!
```

## 4. 使用 clang 简化流程（推荐）

也可以直接使用 `clang` 来编译 LLVM IR 文件，它会自动完成目标代码生成和链接：

```bash
clang hello_world.ll -o hello_world.exe
```

## 5. 使用 GCC (MinGW) 编译

如果你使用 MinGW，可以使用 `gcc`：

```bash
gcc hello_world.ll -o hello_world.exe
```

## 6. 完整示例

让我们用 `hello_world.ch` 示例走一遍完整流程：

### 示例代码（hello_world.ch）

```c
extern "C" {
    func printf(byte^ format, ...) -> int;
}

func main() -> int {
    printf("Hello, World!\n");
    return 0;
}
```

### 编译命令

```bash
# 1. 进入项目目录
cd d:\projects\CppProjs\c-hat

# 2. 生成 LLVM IR
.\build\src\Debug\c_hat_parser.exe tests\hello_world.ch --emit-llvm

# 3. 使用 llc 生成目标文件
D:\vcpkg\installed\x64-windows\tools\llvm\llc.exe -filetype=obj hello_world.ll -o hello_world.obj

# 4. 设置 VS 环境变量（如果需要）
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# 5. 链接
link.exe hello_world.obj /OUT:hello_world.exe /SUBSYSTEM:CONSOLE msvcrt.lib

# 6. 运行
hello_world.exe
```

## 7. C 标准库互操作

从上面的示例可以看到，我们使用 `extern "C"` 块来声明 C 标准库函数：

```c
extern "C" {
    func printf(byte^ format, ...) -> int;
    func malloc(size_t size) -> byte^;
    func free(byte^ ptr);
}
```

这样声明的函数会被正确链接到 C 运行时库。

## 8. 未来计划

1. **集成目标代码生成**：直接在编译器中使用 LLVM API 生成目标文件，无需外部 `llc`
2. **自动链接**：集成链接器，一步生成可执行程序
3. **标准库封装**：提供更友好的 C 标准库封装，无需手动 `extern "C"` 声明
4. **错误处理**：完善编译过程中的错误报告和诊断信息

## 9. 相关资源

- [LLVM 官方文档](https://llvm.org/docs/)
- [LLVM IR 语言参考](https://llvm.org/docs/LangRef.html)
- [LLVM 命令行工具](https://llvm.org/docs/CommandGuide/index.html)
