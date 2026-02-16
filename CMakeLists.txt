cmake_minimum_required(VERSION 3.16)
project(c_hat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 LLVM 包
find_package(LLVM REQUIRED CONFIG)
if(LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "LLVM include directories: ${LLVM_INCLUDE_DIRS}")
    message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")
    
    # 使用 llvm_map_components_to_libnames 获取组件库
    llvm_map_components_to_libnames(LLVM_LIBS
        core
        support
        irreader
        passes
        analysis
        codegen
        target
        mc
        asmparser
        asmprinter
        scalaropts
        transformutils
        bitreader
        bitwriter
    )
else()
    message(FATAL_ERROR "LLVM not found. Please install LLVM.")
endif()

# Windows 特定编译定义
if(WIN32)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )
endif()

# 全局编译警告选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 添加 src 子目录
add_subdirectory(src)

# 添加 tests 子目录
add_subdirectory(tests)
