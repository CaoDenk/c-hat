find_package(argparse CONFIG REQUIRED)

add_subdirectory(lexer)
add_subdirectory(ast)
add_subdirectory(parser)
add_subdirectory(types)
add_subdirectory(semantic)
add_subdirectory(codegen)
add_subdirectory(llvm)

add_executable(c_hat_parser main.cpp)
target_include_directories(c_hat_parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(c_hat_parser PRIVATE lexer ast parser types semantic code_generator llvm_codegen argparse::argparse)

# 显式添加 diaguids.lib 链接（如果存在）
if(WIN32)
    if(EXISTS "C:/Program Files/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64/diaguids.lib")
        target_link_libraries(c_hat_parser PRIVATE "C:/Program Files/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64/diaguids.lib")
        message(STATUS "Found diaguids.lib: C:/Program Files/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64/diaguids.lib")
    elseif(EXISTS "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib/amd64/diaguids.lib")
        target_link_libraries(c_hat_parser PRIVATE "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib/amd64/diaguids.lib")
        message(STATUS "Found diaguids.lib: C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib/amd64/diaguids.lib")
    endif()
endif()
