cmake_minimum_required(VERSION 3.20)
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
        x86asmparser
        x86codegen
        x86desc
        x86info
        lto
        option
        objcopy
        object
        orcjit
        native
        windowsdriver
    )
    
    # 查找 LLD 库
    find_library(LLD_LIBRARY NAMES lld lldDriver PATHS ${LLVM_LIBRARY_DIRS})
    if(LLD_LIBRARY)
        message(STATUS "Found LLD library: ${LLD_LIBRARY}")
        list(APPEND LLVM_LIBS ${LLD_LIBRARY})
    else()
        # 尝试查找各个 LLD 组件
        find_library(LLD_COFF NAMES lldCOFF PATHS ${LLVM_LIBRARY_DIRS})
        find_library(LLD_COMMON NAMES lldCommon PATHS ${LLVM_LIBRARY_DIRS})
        find_library(LLD_DRIVER NAMES lldDriver PATHS ${LLVM_LIBRARY_DIRS})
        if(LLD_COFF AND LLD_COMMON)
            message(STATUS "Found LLD COFF: ${LLD_COFF}")
            message(STATUS "Found LLD Common: ${LLD_COMMON}")
            list(APPEND LLVM_LIBS ${LLD_COFF} ${LLD_COMMON})
            if(LLD_DRIVER)
                list(APPEND LLVM_LIBS ${LLD_DRIVER})
            endif()
        else()
            message(WARNING "LLD library not found, will use external linker")
        endif()
    endif()
    
    # 查找 LLVMWindowsManifest 库（LLD COFF 需要）
    find_library(LLVM_WINDOWS_MANIFEST NAMES LLVMWindowsManifest PATHS ${LLVM_LIBRARY_DIRS})
    if(LLVM_WINDOWS_MANIFEST)
        message(STATUS "Found LLVMWindowsManifest: ${LLVM_WINDOWS_MANIFEST}")
        list(APPEND LLVM_LIBS ${LLVM_WINDOWS_MANIFEST})
    endif()
    
    # 查找 LLVMlibDriver 库（LLD COFF 需要）
    find_library(LLVM_LIB_DRIVER NAMES LLVMlibDriver PATHS ${LLVM_LIBRARY_DIRS})
    if(LLVM_LIB_DRIVER)
        message(STATUS "Found LLVMlibDriver: ${LLVM_LIB_DRIVER}")
        list(APPEND LLVM_LIBS ${LLVM_LIB_DRIVER})
    endif()
else()
    message(FATAL_ERROR "LLVM not found. Please install LLVM.")
endif()

# Windows 特定编译定义
if(WIN32)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
        NDEBUG
        _NDEBUG
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
