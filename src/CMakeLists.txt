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
