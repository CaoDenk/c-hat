add_library(llvm_codegen STATIC
    LLVMIRGenerator.cpp
    LLVMCodeGenerator.cpp
)

target_include_directories(llvm_codegen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(llvm_codegen PUBLIC ${LLVM_DEFINITIONS})

target_link_libraries(llvm_codegen PUBLIC ast semantic ${LLVM_LIBS})
