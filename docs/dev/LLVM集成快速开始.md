# LLVM 集成快速开始

## 1. 快速安装

### Windows

```powershell
# 方法 1：下载预编译版本（最快）
# 访问：https://github.com/llvm/llvm-project/releases
# 下载 LLVM-18.1.8-win64.exe
# 安装到：D:/LLVM

# 方法 2：使用 vcpkg
vcpkg install llvm:x64-windows

# 验证安装
clang --version
```

### Linux

```bash
# Ubuntu/Debian
sudo apt-get install llvm-18-dev clang-18 lld-18

# 验证安装
clang-18 --version
```

### macOS

```bash
# 使用 Homebrew
brew install llvm@18

# 验证安装
/opt/homebrew/opt/llvm@18/bin/clang --version
```

## 2. 最小化 CMake 配置

```cmake
cmake_minimum_required(VERSION 3.16)
project(c_hat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# 查找 LLVM
find_package(LLVM 18 REQUIRED CONFIG)

# 源文件
set(SOURCE_FILES
    src/main.cpp
    src/llvm/LLVMIRGenerator.cpp
)

# 创建可执行文件
add_executable(c_hat_compiler ${SOURCE_FILES})

# 链接 LLVM
target_link_libraries(c_hat_compiler PRIVATE LLVM::LLVM)

# Windows 特定设置
if(WIN32)
    target_compile_definitions(c_hat_compiler PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()
```

## 3. 最小化代码示例

### 3.1 简单的 LLVM IR 生成器

```cpp
// src/llvm/SimpleIRGenerator.h
#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <string>

class SimpleIRGenerator {
public:
    SimpleIRGenerator(const std::string& moduleName);
    ~SimpleIRGenerator();

    llvm::Module* getModule() { return module.get(); }
    llvm::IRBuilder<>* getBuilder() { return builder.get(); }

    void printIR();

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::Module> module;
};
```

```cpp
// src/llvm/SimpleIRGenerator.cpp
#include "SimpleIRGenerator.h"
#include <llvm/Support/raw_ostream.h>

SimpleIRGenerator::SimpleIRGenerator(const std::string& moduleName)
    : context(std::make_unique<llvm::LLVMContext>()),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)),
      module(std::make_unique<llvm::Module>(moduleName, *context)) {}

SimpleIRGenerator::~SimpleIRGenerator() = default;

void SimpleIRGenerator::printIR() {
    module->print(llvm::outs(), nullptr);
}
```

### 3.2 生成简单的 LLVM IR

```cpp
// src/main.cpp
#include "llvm/SimpleIRGenerator.h"
#include <iostream>

int main() {
    // 创建 IR 生成器
    SimpleIRGenerator generator("test_module");

    // 创建主函数
    auto& context = *generator.getModule()->getContext();
    auto* module = generator.getModule();
    auto* builder = generator.getBuilder();

    // 定义函数类型：int main()
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context), false);

    // 创建函数
    llvm::Function* mainFunc = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        "main",
        module);

    // 创建基本块
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", mainFunc);
    builder->SetInsertPoint(entry);

    // 创建常量 42
    llvm::ConstantInt* fortyTwo = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context), 42);

    // 返回 42
    builder->CreateRet(fortyTwo);

    // 打印生成的 IR
    std::cout << "Generated LLVM IR:" << std::endl;
    generator.printIR();

    return 0;
}
```

## 4. 构建和运行

```bash
# 配置
mkdir build && cd build
cmake ..

# 构建
cmake --build .

# 运行
./c_hat_compiler
```

## 5. 预期输出

```
Generated LLVM IR:
; ModuleID = 'test_module'
source_filename = "test_module"

define i32 @main() {
entry:
  ret i32 42
}
```

## 6. 验证生成的 IR

```bash
# 保存 IR 到文件
./c_hat_compiler > test.ll

# 验证 IR
opt -verify test.ll

# 优化 IR
opt -O2 -S test.ll -o optimized.ll

# 查看优化后的 IR
cat optimized.ll

# 生成汇编代码
llc test.ll -o test.s

# 编译并运行
clang test.s -o test
./test
# 输出：42
```

## 7. 下一步

1. ✅ 完成 LLVM 安装
2. ✅ 创建最小化示例
3. ✅ 生成并验证 LLVM IR
4. ⬜ 扩展 IR 生成器支持更多 C^ 特性
5. ⬜ 添加优化 Pass
6. ⬜ 集成到完整的编译器流程

## 8. 常见问题

### Q: 找不到 LLVM

**A**: 确保 LLVM 已安装并正确配置 CMake

```cmake
# 检查 LLVM 是否找到
message(STATUS "LLVM_DIR: ${LLVM_DIR}")
message(STATUS "LLVM_VERSION: ${LLVM_VERSION}")
```

### Q: 链接错误

**A**: 确保链接了正确的 LLVM 库

```cmake
# 查看可用的 LLVM 库
message(STATUS "LLVM_LIBRARIES: ${LLVM_LIBRARIES}")
```

### Q: IR 验证失败

**A**: 使用验证工具检查 IR

```bash
opt -verify test.ll
```

## 9. 参考资源

- [LLVM 官网](https://llvm.org/)
- [LLVM 教程](https://llvm.org/docs/tutorial/)
- [LLVM API 文档](https://llvm.org/doxygen/)
