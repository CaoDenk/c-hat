# LLVM 集成指南

## 1. LLVM 简介

LLVM (Low Level Virtual Machine) 是一个模块化、可重用的编译器和工具链技术集合，被广泛应用于现代编译器开发中。

### 1.1 为什么选择 LLVM

- **成熟的代码生成后端**：支持多种架构（x86、ARM、RISC-V 等）
- **强大的优化能力**：提供丰富的优化 pass
- **活跃的社区支持**：持续更新和改进
- **优秀的文档**：API 文档齐全
- **跨平台支持**：Windows、Linux、macOS
- **中间表示（IR）**：设计良好的 SSA 形式 IR

### 1.2 LLVM 在 C^ 编译器中的作用

```
C^ 源代码 → 词法分析 → 语法分析 → 语义分析 → LLVM IR → 优化 → 目标代码
```

## 2. 安装和配置 LLVM

### 2.1 Windows 环境

#### 方法 1：使用预编译二进制文件（推荐）

```powershell
# 下载 LLVM 预编译版本
# 访问：https://github.com/llvm/llvm-project/releases
# 下载 LLVM-18.1.8-win64.exe 或 LLVM-18.1.8-win64.7z

# 安装到：D:/LLVM
# 安装后添加到 PATH
$env:PATH += ";D:/LLVM/bin"

# 验证安装
clang --version
llc --version
opt --version
```

#### 方法 2：使用 vcpkg

```powershell
# 安装 LLVM
vcpkg install llvm:x64-windows

# 安装后路径
# D:/vcpkg/installed/x64-windows/include/llvm
# D:/vcpkg/installed/x64-windows/lib/
```

#### 方法 3：从源码编译

```powershell
# 克隆 LLVM 源码
git clone https://github.com/llvm/llvm-project.git
cd llvm-project

# 创建构建目录
mkdir build
cd build

# 配置（使用 Ninja）
cmake -G Ninja ^
  -DLLVM_ENABLE_PROJECTS="clang;lld" ^
  -DLLVM_TARGETS_TO_BUILD="X86;ARM;AArch64;RISCV" ^
  -DLLVM_BUILD_EXAMPLES=OFF ^
  -DLLVM_BUILD_TESTS=OFF ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=D:/LLVM ^
  ../llvm

# 编译（需要较长时间）
ninja
ninja install
```

### 2.2 Linux 环境

```bash
# Ubuntu/Debian
sudo apt-get install llvm-18-dev clang-18 lld-18

# 或从源码编译
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build && cd build
cmake -G Ninja \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_TARGETS_TO_BUILD="X86;ARM;AArch64;RISCV" \
  -DLLVM_BUILD_EXAMPLES=OFF \
  -DLLVM_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  ../llvm
ninja
sudo ninja install
```

### 2.3 macOS 环境

```bash
# 使用 Homebrew
brew install llvm@18

# 设置环境变量
echo 'export PATH="/usr/local/opt/llvm@18/bin:$PATH"' >> ~/.zshrc
echo 'export LDFLAGS="-L/usr/local/opt/llvm@18/lib"' >> ~/.zshrc
echo 'export CPPFLAGS="-I/usr/local/opt/llvm@18/include"' >> ~/.zshrc
source ~/.zshrc
```

## 3. 集成 LLVM 到 CMake 项目

### 3.1 更新 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(c_hat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 LLVM 包
find_package(LLVM 18 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# 添加 LLVM 头文件路径
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# 链接 LLVM 库
llvm_map_components_to_libnames(llvm_libs
  support
  core
  executionengine
  interpreter
  mc
  transformutils
  analysis
  instcombine
  scalaropts
  target
  targetparser
  x86asmparser
  x86codegen
  x86desc
  x86info
  irreader
  codegen
)

# 源文件
set(SOURCE_FILES
  src/lexer/Lexer.cpp
  src/ast/AstNodes.cpp
  src/parser/Parser.cpp
  src/types/Types.cpp
  src/semantic/SymbolTable.cpp
  src/semantic/SemanticAnalyzer.cpp
  src/codegen/CodeGenerator.cpp
  src/llvm/LLVMIRGenerator.cpp
  src/main.cpp
)

# 创建可执行文件
add_executable(c_hat_compiler ${SOURCE_FILES})

# 链接 LLVM 库
target_link_libraries(c_hat_compiler PRIVATE ${llvm_libs})

# Windows 特定设置
if(WIN32)
    target_compile_definitions(c_hat_compiler PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )
endif()

# 编译警告选项
if(MSVC)
    target_compile_options(c_hat_compiler PRIVATE /W4)
else()
    target_compile_options(c_hat_compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

### 3.2 使用 vcpkg 的 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(c_hat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用 vcpkg 工具链
find_package(LLVM CONFIG REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# 源文件
set(SOURCE_FILES
  src/lexer/Lexer.cpp
  src/ast/AstNodes.cpp
  src/parser/Parser.cpp
  src/types/Types.cpp
  src/semantic/SymbolTable.cpp
  src/semantic/SemanticAnalyzer.cpp
  src/codegen/CodeGenerator.cpp
  src/llvm/LLVMIRGenerator.cpp
  src/main.cpp
)

# 创建可执行文件
add_executable(c_hat_compiler ${SOURCE_FILES})

# 链接 LLVM 库
target_link_libraries(c_hat_compiler PRIVATE LLVM::LLVM)

# Windows 特定设置
if(WIN32)
    target_compile_definitions(c_hat_compiler PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )
endif()
```

## 4. LLVM IR 生成实现

### 4.1 创建 LLVM IR 生成器

```cpp
// src/llvm/LLVMIRGenerator.h
#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>

namespace c_hat::llvm_codegen {

class LLVMIRGenerator {
public:
    LLVMIRGenerator(const std::string& moduleName);
    ~LLVMIRGenerator();

    // 初始化 LLVM 目标
    static void initializeLLVM();

    // 生成 LLVM IR
    void generateFromAST(const ast::Node* root);

    // 获取生成的模块
    llvm::Module* getModule() { return module.get(); }

    // 验证 IR
    bool verifyIR();

    // 打印 IR
    void printIR();

    // 写入 IR 到文件
    bool writeIRToFile(const std::string& filename);

    // 生成目标代码
    bool generateObjectFile(const std::string& filename);

    // 生成汇编代码
    bool generateAssemblyFile(const std::string& filename);

    // JIT 执行
    bool executeJIT();

private:
    // LLVM 核心组件
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::Module> module;

    // 目标机器
    llvm::TargetMachine* targetMachine;

    // 符号表
    std::map<std::string, llvm::Value*> symbolTable;

    // 类型映射
    llvm::Type* mapType(const types::Type* type);

    // AST 遍历方法
    void visitNode(const ast::Node* node);
    void visitFunctionDecl(const ast::FunctionDecl* func);
    void visitVariableDecl(const ast::VariableDecl* var);
    void visitReturnStmt(const ast::ReturnStmt* ret);
    void visitBinaryExpr(const ast::BinaryExpr* expr);
    void visitCallExpr(const ast::CallExpr* expr);
    void visitLiteral(const ast::Literal* lit);
};

} // namespace c_hat::llvm_codegen
```

### 4.2 实现 LLVM IR 生成器

```cpp
// src/llvm/LLVMIRGenerator.cpp
#include "LLVMIRGenerator.h"
#include "../ast/AstNodes.h"
#include "../types/Types.h"
#include <iostream>

namespace c_hat::llvm_codegen {

// 初始化 LLVM 目标
void LLVMIRGenerator::initializeLLVM() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
}

LLVMIRGenerator::LLVMIRGenerator(const std::string& moduleName)
    : context(std::make_unique<llvm::LLVMContext>()),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)),
      module(std::make_unique<llvm::Module>(moduleName, *context)),
      targetMachine(nullptr) {

    // 设置目标机器
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        std::cerr << "Failed to lookup target: " << error << std::endl;
        return;
    }

    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions opt;
    auto rm = llvm::Reloc::Model::PIC_;

    targetMachine = target->createTargetMachine(
        targetTriple, cpu, features, opt, rm);

    module->setDataLayout(targetMachine->createDataLayout());
}

LLVMIRGenerator::~LLVMIRGenerator() = default;

// 类型映射
llvm::Type* LLVMIRGenerator::mapType(const types::Type* type) {
    if (!type) return nullptr;

    switch (type->getKind()) {
        case types::Type::Kind::Primitive: {
            auto primitive = static_cast<const types::PrimitiveType*>(type);
            switch (primitive->getPrimitiveKind()) {
                case types::PrimitiveType::Kind::Void:
                    return llvm::Type::getVoidTy(*context);
                case types::PrimitiveType::Kind::Int:
                case types::PrimitiveType::Kind::Int8:
                case types::PrimitiveType::Kind::Int16:
                case types::PrimitiveType::Kind::Int32:
                    return llvm::Type::getInt32Ty(*context);
                case types::PrimitiveType::Kind::Int64:
                case types::PrimitiveType::Kind::Long:
                    return llvm::Type::getInt64Ty(*context);
                case types::PrimitiveType::Kind::UInt:
                case types::PrimitiveType::Kind::UInt8:
                case types::PrimitiveType::Kind::UInt16:
                case types::PrimitiveType::Kind::UInt32:
                    return llvm::Type::getInt32Ty(*context);
                case types::PrimitiveType::Kind::UInt64:
                case types::PrimitiveType::Kind::ULong:
                    return llvm::Type::getInt64Ty(*context);
                case types::PrimitiveType::Kind::Float:
                    return llvm::Type::getFloatTy(*context);
                case types::PrimitiveType::Kind::Double:
                    return llvm::Type::getDoubleTy(*context);
                case types::PrimitiveType::Kind::Bool:
                    return llvm::Type::getInt1Ty(*context);
                case types::PrimitiveType::Kind::Char:
                    return llvm::Type::getInt32Ty(*context);
                default:
                    return nullptr;
            }
        }
        case types::Type::Kind::Pointer: {
            auto pointer = static_cast<const types::PointerType*>(type);
            auto elementType = mapType(pointer->getElementType());
            return elementType->getPointerTo();
        }
        case types::Type::Kind::Array: {
            auto array = static_cast<const types::ArrayType*>(type);
            auto elementType = mapType(array->getElementType());
            return llvm::ArrayType::get(elementType, array->getSize());
        }
        case types::Type::Kind::Function: {
            auto func = static_cast<const types::FunctionType*>(type);
            auto returnType = mapType(func->getReturnType());
            std::vector<llvm::Type*> paramTypes;
            for (const auto& param : func->getParameterTypes()) {
                paramTypes.push_back(mapType(param));
            }
            return llvm::FunctionType::get(returnType, paramTypes, false);
        }
        default:
            return nullptr;
    }
}

// 访问函数声明
void LLVMIRGenerator::visitFunctionDecl(const ast::FunctionDecl* func) {
    // 获取函数类型
    auto funcType = static_cast<llvm::FunctionType*>(mapType(func->getType()));

    // 创建函数
    auto function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        func->getName(),
        module.get());

    // 设置参数名称
    size_t idx = 0;
    for (auto& arg : function->args()) {
        arg.setName(func->getParameters()[idx++]->getName());
    }

    // 创建基本块
    auto block = llvm::BasicBlock::Create(*context, "entry", function);
    builder->SetInsertPoint(block);

    // 保存参数到符号表
    idx = 0;
    for (auto& arg : function->args()) {
        symbolTable[func->getParameters()[idx++]->getName()] = &arg;
    }

    // 生成函数体
    visitNode(func->getBody());

    // 验证函数
    llvm::verifyFunction(*function);
}

// 访问变量声明
void LLVMIRGenerator::visitVariableDecl(const ast::VariableDecl* var) {
    auto type = mapType(var->getType());
    auto alloca = builder->CreateAlloca(type, nullptr, var->getName());

    if (var->getInitializer()) {
        auto initValue = visitNode(var->getInitializer());
        builder->CreateStore(initValue, alloca);
    }

    symbolTable[var->getName()] = alloca;
}

// 访问返回语句
void LLVMIRGenerator::visitReturnStmt(const ast::ReturnStmt* ret) {
    if (ret->getValue()) {
        auto value = visitNode(ret->getValue());
        builder->CreateRet(value);
    } else {
        builder->CreateRetVoid();
    }
}

// 访问二元表达式
void LLVMIRGenerator::visitBinaryExpr(const ast::BinaryExpr* expr) {
    auto left = visitNode(expr->getLeft());
    auto right = visitNode(expr->getRight());

    switch (expr->getOperator()) {
        case ast::BinaryExpr::Operator::Add:
            return builder->CreateAdd(left, right, "addtmp");
        case ast::BinaryExpr::Operator::Sub:
            return builder->CreateSub(left, right, "subtmp");
        case ast::BinaryExpr::Operator::Mul:
            return builder->CreateMul(left, right, "multmp");
        case ast::BinaryExpr::Operator::Div:
            return builder->CreateSDiv(left, right, "divtmp");
        case ast::BinaryExpr::Operator::Equal:
            return builder->CreateICmpEQ(left, right, "eqtmp");
        case ast::BinaryExpr::Operator::NotEqual:
            return builder->CreateICmpNE(left, right, "netmp");
        case ast::BinaryExpr::Operator::Less:
            return builder->CreateICmpSLT(left, right, "lttmp");
        case ast::BinaryExpr::Operator::LessEqual:
            return builder->CreateICmpSLE(left, right, "letmp");
        case ast::BinaryExpr::Operator::Greater:
            return builder->CreateICmpSGT(left, right, "gttmp");
        case ast::BinaryExpr::Operator::GreaterEqual:
            return builder->CreateICmpSGE(left, right, "getmp");
        default:
            return nullptr;
    }
}

// 访问调用表达式
void LLVMIRGenerator::visitCallExpr(const ast::CallExpr* expr) {
    auto callee = module->getFunction(expr->getCalleeName());
    if (!callee) {
        std::cerr << "Unknown function referenced: " << expr->getCalleeName() << std::endl;
        return nullptr;
    }

    std::vector<llvm::Value*> args;
    for (const auto& arg : expr->getArguments()) {
        args.push_back(visitNode(arg));
    }

    return builder->CreateCall(callee, args, "calltmp");
}

// 访问字面量
void LLVMIRGenerator::visitLiteral(const ast::Literal* lit) {
    switch (lit->getKind()) {
        case ast::Literal::Kind::Integer:
            return llvm::ConstantInt::get(
                *context,
                llvm::APInt(32, std::stoi(lit->getValue())));
        case ast::Literal::Kind::Float:
            return llvm::ConstantFP::get(
                *context,
                llvm::APFloat(std::stod(lit->getValue())));
        case ast::Literal::Kind::Boolean:
            return llvm::ConstantInt::get(
                *context,
                llvm::APInt(1, lit->getValue() == "true"));
        case ast::Literal::Kind::String:
            return builder->CreateGlobalStringPtr(lit->getValue());
        default:
            return nullptr;
    }
}

// 验证 IR
bool LLVMIRGenerator::verifyIR() {
    std::string error;
    llvm::raw_string_ostream os(error);
    if (llvm::verifyModule(*module, &os)) {
        std::cerr << "IR verification failed: " << error << std::endl;
        return false;
    }
    return true;
}

// 打印 IR
void LLVMIRGenerator::printIR() {
    module->print(llvm::outs(), nullptr);
}

// 写入 IR 到文件
bool LLVMIRGenerator::writeIRToFile(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream file(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        std::cerr << "Error opening file: " << ec.message() << std::endl;
        return false;
    }

    module->print(file, nullptr);
    return true;
}

// 生成目标代码
bool LLVMIRGenerator::generateObjectFile(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream file(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        std::cerr << "Error opening file: " << ec.message() << std::endl;
        return false;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CGFT_ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, file, nullptr, fileType)) {
        std::cerr << "TargetMachine can't emit a file of this type" << std::endl;
        return false;
    }

    pass.run(*module);
    file.flush();
    return true;
}

// 生成汇编代码
bool LLVMIRGenerator::generateAssemblyFile(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream file(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        std::cerr << "Error opening file: " << ec.message() << std::endl;
        return false;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CGFT_AssemblyFile;

    if (targetMachine->addPassesToEmitFile(pass, file, nullptr, fileType)) {
        std::cerr << "TargetMachine can't emit a file of this type" << std::endl;
        return false;
    }

    pass.run(*module);
    file.flush();
    return true;
}

} // namespace c_hat::llvm_codegen
```

## 5. 优化 Pass 配置

### 5.1 基础优化 Pass

```cpp
// src/llvm/LLVMOptimizer.h
#pragma once

#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <memory>

namespace c_hat::llvm_codegen {

class LLVMOptimizer {
public:
    LLVMOptimizer(llvm::Module* module);
    ~LLVMOptimizer() = default;

    // 运行优化
    void optimize(int optimizationLevel);

    // 设置优化级别
    void setOptimizationLevel(int level) { optLevel = level; }

private:
    llvm::Module* module;
    int optLevel;

    // 添加优化 Pass
    void addOptimizationPasses(llvm::PassBuilder& pb,
                                llvm::FunctionPassManager& fpm,
                                llvm::LoopPassManager& lpm,
                                llvm::ModulePassManager& mpm);
};

} // namespace c_hat::llvm_codegen
```

### 5.2 优化 Pass 实现

```cpp
// src/llvm/LLVMOptimizer.cpp
#include "LLVMOptimizer.h"
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Target/TargetMachine.h>

namespace c_hat::llvm_codegen {

LLVMOptimizer::LLVMOptimizer(llvm::Module* module)
    : module(module), optLevel(2) {}

void LLVMOptimizer::optimize(int optimizationLevel) {
    optLevel = optimizationLevel;

    llvm::PassBuilder pb;
    llvm::FunctionPassManager fpm;
    llvm::LoopPassManager lpm;
    llvm::ModulePassManager mpm;

    addOptimizationPasses(pb, fpm, lpm, mpm);

    // 运行优化
    mpm.run(*module, pb);
}

void LLVMOptimizer::addOptimizationPasses(llvm::PassBuilder& pb,
                                           llvm::FunctionPassManager& fpm,
                                           llvm::LoopPassManager& lpm,
                                           llvm::ModulePassManager& mpm) {
    // 根据优化级别添加 Pass
    if (optLevel >= 1) {
        // O1 优化
        fpm.addPass(llvm::InstCombinePass());
        fpm.addPass(llvm::PromotePass());
        fpm.addPass(llvm::SimplifyCFGPass());
    }

    if (optLevel >= 2) {
        // O2 优化
        fpm.addPass(llvm::ReassociatePass());
        fpm.addPass(llvm::GVNPass());
        fpm.addPass(llvm::NewGVNPass());
        fpm.addPass(llvm::DCEPass());
        fpm.addPass(llvm::SROAPass());
        fpm.addPass(llvm::EarlyCSEPass());
        fpm.addPass(llvm::SpeculativeExecutionPass());
        fpm.addPass(llvm::JumpThreadingPass());
        fpm.addPass(llvm::CorrelatedValuePropagationPass());
        fpm.addPass(llvm::AggressiveDCEPass());
    }

    if (optLevel >= 3) {
        // O3 优化
        fpm.addPass(llvm::LoopVectorizePass());
        fpm.addPass(llvm::SLPVectorizerPass());
        fpm.addPass(llvm::LoopUnrollPass());
        fpm.addPass(llvm::LoopUnrollAndJamPass());
        fpm.addPass(llvm:: LICMPass());
        fpm.addPass(llvm::IndVarSimplifyPass());
        fpm.addPass(llvm::LoopIdiomPass());
    }

    // 模块级优化
    mpm.addPass(llvm::GlobalOptPass());
    mpm.addPass(llvm::GlobalDCEPass());
    mpm.addPass(llvm::StripDeadPrototypesPass());
}

} // namespace c_hat::llvm_codegen
```

## 6. 调试和验证

### 6.1 IR 验证

```cpp
// 在生成 IR 后验证
if (!irGenerator->verifyIR()) {
    std::cerr << "Generated IR is invalid!" << std::endl;
    return 1;
}

// 打印 IR 用于调试
irGenerator->printIR();

// 写入 IR 到文件
irGenerator->writeIRToFile("output.ll");
```

### 6.2 使用 opt 工具

```bash
# 查看 IR
opt -S input.ll -o output.ll

# 运行优化
opt -O2 -S input.ll -o optimized.ll

# 查看特定 Pass
opt -passes=instcombine -S input.ll -o output.ll

# 验证 IR
opt -verify input.ll
```

### 6.3 使用 llc 工具

```bash
# 生成汇编代码
llc -O2 input.ll -o output.s

# 生成目标文件
llc -filetype=obj input.ll -o output.o

# 查看生成的汇编
llc -O2 -S input.ll
```

### 6.4 使用 lli 工具（JIT）

```bash
# JIT 执行
lli input.ll

# 带参数
lli input.ll arg1 arg2
```

## 7. 常见问题和解决方案

### 7.1 链接错误

**问题**：无法解析的外部符号

**解决方案**：
```cmake
# 确保链接了所有需要的 LLVM 组件
llvm_map_components_to_libnames(llvm_libs
  support
  core
  executionengine
  interpreter
  mc
  transformutils
  analysis
  instcombine
  scalaropts
  target
  targetparser
  x86asmparser
  x86codegen
  x86desc
  x86info
  irreader
  codegen
)
```

### 7.2 版本不匹配

**问题**：LLVM 版本不兼容

**解决方案**：
```cmake
# 指定 LLVM 版本
find_package(LLVM 18 REQUIRED CONFIG)

# 或使用 vcpkg 确保版本一致
vcpkg install llvm:x64-windows
```

### 7.3 IR 验证失败

**问题**：生成的 IR 验证失败

**解决方案**：
```cpp
// 使用调试模式
llvm::DebugFlag = true;

// 打印详细的错误信息
std::string error;
llvm::raw_string_ostream os(error);
if (llvm::verifyModule(*module, &os)) {
    std::cerr << "IR verification failed: " << error << std::endl;
    module->print(llvm::errs(), nullptr);
}
```

### 7.4 性能问题

**问题**：编译速度慢

**解决方案**：
```cpp
// 使用 Release 模式编译
set(CMAKE_BUILD_TYPE Release)

// 禁用调试信息
add_definitions(-DNDEBUG)

// 使用 LTO（链接时优化）
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
```

## 8. 最佳实践

### 8.1 内存管理

```cpp
// 使用智能指针管理 LLVM 对象
std::unique_ptr<llvm::LLVMContext> context;
std::unique_ptr<llvm::IRBuilder<>> builder;
std::unique_ptr<llvm::Module> module;

// LLVM 对象的生命周期由 LLVMContext 管理
// 不要手动删除 LLVM 对象
```

### 8.2 错误处理

```cpp
// 使用 LLVM 的错误处理机制
llvm::Error err = someFunction();
if (err) {
    llvm::logAllUnhandledErrors(std::move(err), llvm::errs(), "Error: ");
    return;
}

// 或使用 Expected
llvm::Expected<llvm::Value*> result = someFunction();
if (!result) {
    llvm::handleAllErrors(result.takeError(), [](const llvm::ErrorInfoBase& e) {
        std::cerr << e.message() << std::endl;
    });
    return;
}
```

### 8.3 命名约定

```cpp
// LLVM IR 中的命名
// - 全局变量：@name
// - 函数：@name
// - 局部变量：%name
// - 标签：name:

// 设置 LLVM IR 中的名称
auto function = llvm::Function::Create(..., "myFunction");
auto alloca = builder->CreateAlloca(type, nullptr, "myVar");
auto add = builder->CreateAdd(left, right, "addtmp");
```

### 8.4 调试信息

```cpp
// 添加调试信息
#include <llvm/IR/DIBuilder.h>

llvm::DIBuilder dibuilder(*module);

// 创建编译单元
auto file = dibuilder.createFile("source.ch", "/path/to/source");
auto cu = dibuilder.createCompileUnit(
    llvm::dwarf::DW_LANG_C,
    file,
    "C^ Compiler",
    true,
    "",
    0);

// 创建函数调试信息
auto subroutine = dibuilder.createFunction(
    cu,
    "myFunction",
    "myFunction",
    file,
    1,
    functionType,
    1,
    llvm::DINode::FlagPrototyped,
    llvm::DISubprogram::SPFlagDefinition);

dibuilder.finalize();
```

## 9. 进阶主题

### 9.1 自定义 Pass

```cpp
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Passes/PassBuilder.h>

struct MyPass : llvm::PassInfoMixin<MyPass> {
    llvm::PreservedAnalyses run(llvm::Function& F,
                                  llvm::FunctionAnalysisManager& AM) {
        // 自定义优化逻辑
        for (auto& BB : F) {
            for (auto& I : BB) {
                // 处理指令
            }
        }

        return llvm::PreservedAnalyses::all();
    }
};

// 注册 Pass
llvm::PassPluginLibraryInfo getMyPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "MyPass", LLVM_VERSION_STRING,
            [](llvm::PassBuilder& PB) {
                PB.registerPipelineParsingCallback(
                    [](llvm::StringRef Name, llvm::FunctionPassManager& FPM,
                       llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                        if (Name == "my-pass") {
                            FPM.addPass(MyPass());
                            return true;
                        }
                        return false;
                    });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getMyPassPluginInfo();
}
```

### 9.2 JIT 编译

```cpp
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

llvm::Expected<std::unique_ptr<llvm::orc::LLJIT>> createJIT() {
    auto jit = llvm::orc::LLJITBuilder().create();
    if (!jit) {
        return jit.takeError();
    }

    // 添加模块
    auto tsm = llvm::orc::ThreadSafeModule(
        std::move(module),
        std::move(context));

    if (auto err = jit->addIRModule(std::move(tsm))) {
        return std::move(err);
    }

    return jit;
}

// 查找并执行函数
auto jit = createJIT();
if (!jit) {
    llvm::logAllUnhandledErrors(jit.takeError(), llvm::errs(), "JIT Error: ");
    return 1;
}

auto symbol = (*jit)->lookup("main");
if (!symbol) {
    llvm::logAllUnhandledErrors(symbol.takeError(), llvm::errs(), "Lookup Error: ");
    return 1;
}

auto mainFunc = (int(*)())symbol->toPtr<int(*)()>();
int result = mainFunc();
```

### 9.3 目标特定优化

```cpp
// 获取目标特定信息
auto targetTriple = llvm::sys::getDefaultTargetTriple();
auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

auto cpu = llvm::sys::getHostCPUName();
auto features = llvm::sys::getHostCPUFeatures();

// 创建目标机器
auto targetMachine = target->createTargetMachine(
    targetTriple,
    cpu,
    features.getString(),
    opt,
    rm);

// 使用目标特定优化
auto targetTransformInfo = targetMachine->getTargetTransformInfo(*function);
```

## 10. 参考资源

### 10.1 官方文档

- [LLVM 官网](https://llvm.org/)
- [LLVM 文档](https://llvm.org/docs/)
- [LLVM API 文档](https://llvm.org/doxygen/)
- [LLVM 教程](https://llvm.org/docs/tutorial/)

### 10.2 学习资源

- [Kaleidoscope 教程](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html)
- [LLVM Cookbook](https://llvm.org/docs/CookBook.html)
- [Writing an LLVM Pass](https://llvm.org/docs/WritingAnLLVMPass.html)

### 10.3 社区资源

- [LLVM Discourse](https://discourse.llvm.org/)
- [LLVM GitHub](https://github.com/llvm/llvm-project)
- [LLVM Weekly](https://llvmweekly.org/)

## 11. 总结

集成 LLVM 到 C^ 编译器项目是一个复杂但值得的过程。通过本指南，你应该能够：

1. ✅ 安装和配置 LLVM
2. ✅ 集成 LLVM 到 CMake 项目
3. ✅ 生成 LLVM IR
4. ✅ 配置优化 Pass
5. ✅ 调试和验证生成的代码
6. ✅ 解决常见问题

记住，LLVM 是一个强大的工具，但也需要深入理解。建议从简单的例子开始，逐步增加复杂性。
